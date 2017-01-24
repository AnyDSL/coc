#include <functional>

#include "thorin/world.h"

namespace thorin {

/*
 * WorldBase
 */

bool WorldBase::alloc_guard_ = false;

WorldBase::WorldBase()
    : root_page_(new Page)
    , cur_page_(root_page_.get())
    , universe_(build_array_nullary<Universe>())
    , star_(build_array_nullary<Star>())
    , space_(build_array_nullary<Space>())
{}

WorldBase::~WorldBase() {
    for (auto def : defs_)
        def->~Def();
}

const Def* WorldBase::index(size_t i, size_t arity, Qualifier q, Debug dbg) {
    auto a = this->arity(arity, q);
    if (i < arity)
        return unify<Index>(0, *this, a, i, dbg);
    return error(a);
}

const Def* WorldBase::variadic_tuple(const Def* dimension, const Def* body, Debug dbg) {
    return type_variadic_tuple(variadic_sigma(dimension, body->type(), dbg), body, dbg);
}

const Pi* WorldBase::pi(Defs domains, const Def* body, Qualifier q, Debug dbg) {
    if (domains.size() == 1) {
        if (auto sigma = domains.front()->isa<Sigma>())
            return pi(sigma->ops(), body, q, dbg);
    }

    return unify<Pi>(domains.size() + 1, *this, domains, body, q, dbg);
}

const Lambda* WorldBase::pi_lambda(const Pi* pi, const Def* body, Debug dbg) {
    assert(pi->body() == body->type());
    return unify<Lambda>(1, *this, pi, body, dbg);
}

const Def* single_qualified(Defs defs, Qualifier q) {
    assert(defs.size() == 1);
    auto single = defs.front();
    // TODO if we use automatic qualifier coercion/subtyping, need to allow it here as well
    assert(!single || single->qualifier() == q);
    return single;
}

const Def* WorldBase::sigma(Defs defs, Qualifier q, Debug dbg) {
    // TODO doing the following always loses Debug information
    if (defs.size() == 1) { return single_qualified(defs, q); }

    return unify<Sigma>(defs.size(), *this, defs, q, dbg);
}

const Def* WorldBase::singleton(const Def* def, Debug dbg) {
    assert(def->type() && "Can't create singletons of universes.");

    if (def->type()->isa<Singleton>())
        return def->type();

    if (!def->is_nominal()) {
        if (def->isa<Variant>()) {
            auto ops = Array<const Def*>(def->num_ops(), [&](auto i) { return this->singleton(def->op(i)); });
            return variant(ops, dbg);
        }

        if (def->isa<Intersection>()) {
            // S(v : t ∩ u) : *
            // TODO Any normalization of a Singleton Intersection?
        }
    }

    if (auto sig = def->type()->isa<Sigma>()) {
        // See Harper PFPL 43.13b
        auto ops = Array<const Def*>(sig->num_ops(), [&](auto i) { return this->singleton(this->extracti(def, i)); });
        return sigma(ops, sig->qualifier(), dbg);
    }

    if (auto pi_type = def->type()->isa<Pi>()) {
        // See Harper PFPL 43.13c
        auto domains = pi_type->domains();
        auto num_domains = pi_type->num_domains();
        auto new_pi_vars = Array<const Def*>(num_domains,
                [&](auto i) { return this->var(domains[i], num_domains - i - 1); });
        auto applied = app(def, new_pi_vars);
        return pi(domains, singleton(applied), pi_type->qualifier(), dbg);
    }

    // TODO other normalizations?
    return unify<Singleton>(1, *this, def, dbg);
}

const Def* WorldBase::tuple(const Def* type, Defs defs, Debug dbg) {
    // will return the single type in case of a single def
    auto expected_type = sigma(types(defs));
    const Def* found_structural_type = type;
    // if defs is just one, we may not have a sigma to unpack
    if (type->is_nominal() && type->isa<Sigma>())
        found_structural_type = sigma(type->ops());
    // TODO subtyping check instead of equality here
    // TODO error message with more precise information
    assert(found_structural_type == expected_type && "Can't give type to tuple.");
    if (!type->is_nominal() && defs.size() == 1) {
        return defs.front();
    }

    return unify<Tuple>(defs.size(), *this, type->as<Sigma>(), defs, dbg);
}

const Def* WorldBase::extract(const Def* def, const Def* i, Debug dbg) {
    if (auto index = i->isa<Index>())
        return extracti(def, index->index(), dbg);
    return unify<Extract>(2, *this, extract(def, i, dbg), def, i, dbg);
}

const Def* WorldBase::extracti(const Def* def, size_t i, Debug dbg) {
    if (auto sigma = def->type()->isa<Sigma>()) {
        if (auto tuple = def->isa<Tuple>())
            return tuple->op(i);

        auto extract_type = [&]() {
            auto type = sigma->op(i);
            if (type->free_vars().none_end(i))
                return type;

            size_t skipped_shifts = 0;
            for (size_t delta = 1; delta <= i; delta++) {
                if (type->free_vars().none_begin(skipped_shifts)) {
                    skipped_shifts++;
                    continue;
                }
                auto prev_extract = extracti(def, i - delta);

                // This also shifts any Var with i > skipped_shifts by -1
                type = type->reduce({prev_extract}, skipped_shifts);
            }
            return type;
        };

        auto type = extract_type();
        return unify<Extract>(2, *this, type, def, index(i, sigma->num_ops(), type->qualifier(), dbg), dbg);
    }

    assert(i == 0);
    return def;
}

const Def* WorldBase::intersection(Defs defs, Qualifier q, Debug dbg) {
    if (defs.size() == 1)
        return single_qualified(defs, q);

    return unify<Intersection>(defs.size(), *this, defs, q, dbg);
}

const Def* WorldBase::pick(const Def* type, const Def* def, Debug dbg) {
    if (auto def_type = def->type()->isa<Intersection>()) {
        assert(std::any_of(def_type->ops().begin(), def_type->ops().end(), [&](auto t) { return t == type; })
               && "Picked type must be a part of the intersection type.");

        return unify<Pick>(1, *this, type, def, dbg);
    }

    assert(type == def->type());
    return def;
}

const Def* WorldBase::variant(Defs defs, Qualifier q, Debug dbg) {
    if (defs.size() == 1) { return single_qualified(defs, q); }

    return unify<Variant>(defs.size(), *this, defs, q, dbg);
}

const Def* WorldBase::any(const Def* type, const Def* def, Debug dbg) {
    if (!type->isa<Variant>()) {
        assert(type == def->type());
        return def;
    }

    auto variants = type->ops();
    assert(std::any_of(variants.begin(), variants.end(), [&](auto t){ return t == def->type(); })
           && "Type must be a part of the variant type.");

    return unify<Any>(1, *this, type->as<Variant>(), def, dbg);
}

const Def* build_match_type(WorldBase& w, const Def* /*def*/, const Variant* /*type*/, Defs handlers) {
    auto types = Array<const Def*>(handlers.size(),
            [&](auto i) { return handlers[i]->type()->template as<Pi>()->body(); });
    // We're not actually building a sum type here, we need uniqueness
    unique_gid_sort(&types);
    return w.variant(types);
}

const Def* WorldBase::match(const Def* def, Defs handlers, Debug dbg) {
    auto def_type = def->type();
    if (handlers.size() == 1) {
        assert(!def_type->isa<Variant>());
        return app(handlers.front(), def, dbg);
    }
    auto matched_type = def->type()->as<Variant>();
    assert(def_type->num_ops() == handlers.size() && "Number of handlers does not match number of cases.");

    Array<const Def*> sorted_handlers(handlers);
    std::sort(sorted_handlers.begin(), sorted_handlers.end(),
              [](const Def* a, const Def* b) {
                  auto a_dom = a->type()->as<Pi>()->domain();
                  auto b_dom = b->type()->as<Pi>()->domain();
                  return a_dom->gid() < b_dom->gid(); });
#ifndef NDEBUG
    for (size_t i = 0; i < sorted_handlers.size(); ++i) {
        auto domain = sorted_handlers[i]->type()->as<Pi>()->domain();
        assertf(domain == matched_type->op(i), "Handler {} with domain {} does not match type {}", i, domain,
                matched_type->op(i));
    }
#endif
    if (auto any = def->isa<Any>()) {
        auto any_def = any->def();
        return app(sorted_handlers[any->index()], any_def, dbg);
    }
    auto type = build_match_type(*this, def, matched_type, sorted_handlers);
    return unify<Match>(1, *this, type, def, sorted_handlers, dbg);
}

const Def* WorldBase::app(const Def* callee, Defs args, Debug dbg) {
    if (args.size() == 1) {
        auto single = args.front();
        if (auto tuple = single->isa<Tuple>())
            return app(callee, tuple->ops(), dbg);

        if (auto sigma_type = single->type()->isa<Sigma>()) {
            auto extracts = Array<const Def*>(sigma_type->num_ops(), [&](auto i) { return this->extracti(single, i); });
            return app(callee, extracts, dbg);
        }
    }

    // TODO what if args types don't match the domains? error?
    auto type = callee->type()->as<Pi>()->reduce(args);
    auto app = unify<App>(args.size() + 1, *this, type, callee, args, dbg);
    assert(app->callee() == callee);

    return app->try_reduce();
}

//------------------------------------------------------------------------------

/*
 * World
 */

std::array<const Axiom*, 4> build_axiom(std::function<const Axiom*(Qualifier q)> fn) {
    return array_for_qualifiers<Axiom>(fn);
}

std::array<const Axiom*, 4> build_nat(World& w, size_t width) {
    return array_for_qualifiers<Axiom>([&](auto q){ return w.val_nat(width, q); });
}

World::World()
    : type_nat_(build_axiom([&](auto q) { return this->axiom(this->star(q),{"Nat"}); }))
    , val_nat_({build_nat(*this, 0), build_nat(*this, 1), build_nat(*this, 2), build_nat(*this, 4),
             build_nat(*this, 8), build_nat(*this, 16), build_nat(*this, 32), build_nat(*this, 64)})
    , type_bool_(build_axiom([&](auto q) { return this->axiom(this->star(q),{"Boolean"}); }))
    , val_bool_({build_axiom([&](auto q) { return this->val_bool(false, q); }),
                build_axiom([&](auto q){ return this->val_bool(true, q); })})
    , type_int_(build_axiom([&](auto q){
                  return this->axiom(this->pi({this->type_nat(), this->type_nat()}, this->star(q)), {"int"}); }))
    , type_real_(axiom(pi({type_nat(), type_bool()}, star()),{"real"}))
    , type_mem_(axiom(star(Qualifier::Linear),{"M"}))
    , type_frame_(axiom(star(Qualifier::Linear),{"F"}))
    , type_ptr_(axiom(pi({star(), type_bool()}, star(),{"ptr"})))
    , type_iarithop_(pi({type_nat(), type_nat()}, pi({
            type_int(var(type_nat(), 1), var(type_nat(), 0)),
            type_int(var(type_nat(), 2), var(type_nat(), 1))},
            type_int(var(type_nat(), 3), var(type_nat(), 2)))))
    , type_rarithop_(pi({type_nat(), type_bool()}, pi({
            type_real(var(type_nat(), 1), var(type_nat(), 0)),
            type_real(var(type_nat(), 2), var(type_nat(), 1))},
            type_real(var(type_nat(), 3), var(type_nat(), 2)))))
    //, icmpop_type_(pi({nat(), boolean(), boolean()}, pi(nat(), pi({
            //type_int(var(nat(), 3), var(boolean(), 2), var(boolean(), 1)),
            //type_int(var(nat(), 4), var(boolean(), 3), var(boolean(), 2))},
            //type_int(var(nat(), 5), var(boolean(), 4), var(boolean(), 3))))))
    //, rcmpop_type_(pi({nat(), boolean()}, pi(nat(), pi({
            //type_real(var(nat(), 2), var(boolean(), 1)),
            //type_real(var(nat(), 3), var(boolean(), 2))}, type_uo1()))))
#if 0
#define CODE(x) \
    , x ## _(axiom(iarithop_type_, {# x}))
    THORIN_I_ARITHOP(CODE)
#undef CODE
#define CODE(x) \
    , x ## _(axiom(rarithop_type_, {# x}))
    THORIN_R_ARITHOP(CODE)
#undef CODE
#endif
{}

}
