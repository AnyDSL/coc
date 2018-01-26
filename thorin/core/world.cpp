#include "thorin/core/world.h"

#include "thorin/core/normalize.h"
#include "thorin/frontend/parser.h"

namespace thorin::core {

//------------------------------------------------------------------------------
/*
 * helpers
 */

std::array<const Def*, 2> shape_and_body(World& world, const Def* def) {
    if (auto variadic = def->isa<Variadic>())
        return {variadic->arity(world), variadic->body()};
    return {world.arity(1), def};
}

std::array<const Def*, 2> infer_width_and_shape(World& world, const Def* def) {
    if (auto variadic = def->type()->isa<Variadic>()) {
        if (!variadic->body()->isa<Variadic>())
            return {variadic->body()->as<App>()->arg(), variadic->arity(world)};
        std::vector<const Def*> arities;
        const Def* cur = variadic;
        for (; cur->isa<Variadic>(); cur = cur->as<Variadic>()->body())
            arities.emplace_back(cur->as<Variadic>()->arity(world));
        return {cur->as<App>()->arg(), world.sigma(arities)};
    }
    return {def->type()->as<App>()->arg(), world.arity(1)};
}

//------------------------------------------------------------------------------

World::World() {
    type_nat();
    type_bool();
    type_i_     = axiom(parse(*this, "Πnat. *"), {"int"});
    type_r_     = axiom(parse(*this, "Πnat. *"), {"real"});
    type_ptr_   = axiom(parse(*this, "Π[*, nat]. *"), {"ptr"});
    type_mem_   = axiom(star(QualifierTag::Linear), {"M"});
    type_frame_ = axiom(star(), {"F"});

    auto type_wop  = parse(*this, "Πf: nat. Πw: nat. Πs: 𝕄. Π[   [s;  int w], [s;  int w]].     [s;  int w] ");
    auto type_mop  = parse(*this, "         Πw: nat. Πs: 𝕄. Π[M, [s;  int w], [s;  int w]]. [M, [s;  int w]]");
    auto type_iop  = parse(*this, "         Πw: nat. Πs: 𝕄. Π[   [s;  int w], [s;  int w]].     [s;  int w] ");
    auto type_rop  = parse(*this, "Πf: nat. Πw: nat. Πs: 𝕄. Π[   [s; real w], [s; real w]].     [s; real w] ");
    auto type_icmp = parse(*this, "         Πw: nat. Πs: 𝕄. Π[   [s;  int w], [s;  int w]].     [s; bool]");
    auto type_rcmp = parse(*this, "Πf: nat. Πw: nat. Πs: 𝕄. Π[   [s; real w], [s; real w]].     [s; bool]");

    for (auto o : WOp ()) wop_ [size_t(o)] = axiom(type_wop,  { op2str(o)});
    for (auto o : MOp ()) mop_ [size_t(o)] = axiom(type_mop,  { op2str(o)});
    for (auto o : IOp ()) iop_ [size_t(o)] = axiom(type_iop,  { op2str(o)});
    for (auto o : ROp ()) rop_ [size_t(o)] = axiom(type_rop,  { op2str(o)});
    for (auto o : ICmp()) icmp_[size_t(o)] = axiom(type_icmp, {cmp2str(o)});
    for (auto o : RCmp()) rcmp_[size_t(o)] = axiom(type_rcmp, {cmp2str(o)});

    op_scast_ = axiom(parse(*this, "Π[w: nat, v: nat]. Πs: 𝕄. Π[s;  int w]. [s;  int v]"));
    op_ucast_ = axiom(parse(*this, "Π[w: nat, v: nat]. Πs: 𝕄. Π[s;  int w]. [s;  int v]"));
    op_rcast_ = axiom(parse(*this, "Π[w: nat, v: nat]. Πs: 𝕄. Π[s; real w]. [s; real v]"));
    op_s2r_ =   axiom(parse(*this, "Π[w: nat, v: nat]. Πs: 𝕄. Π[s;  int w]. [s; real v]"));
    op_u2r_ =   axiom(parse(*this, "Π[w: nat, v: nat]. Πs: 𝕄. Π[s;  int w]. [s; real v]"));
    op_r2s_ =   axiom(parse(*this, "Π[w: nat, v: nat]. Πs: 𝕄. Π[s; real w]. [s;  int v]"));
    op_r2u_ =   axiom(parse(*this, "Π[w: nat, v: nat]. Πs: 𝕄. Π[s; real w]. [s;  int v]"));

    op_lea_   = axiom(parse(*this, "Π[s: 𝕄, Ts: [s; *], as: nat]. Π[ptr([j: s; (Ts#j)], as), i: s]. ptr((Ts#i), as)"), {"lea"});
    op_load_  = axiom(parse(*this, "Π[T: *, a: nat]. Π[M, ptr(T, a)]. [M, T]"), {"load"});
    op_store_ = axiom(parse(*this, "Π[T: *, a: nat]. Π[M, ptr(T, a), T]. M"), {"store"});
    op_enter_ = axiom(parse(*this, "ΠM. [M, F]"), {"enter"});
    op_slot_  = axiom(parse(*this, "Π[T: *, a: nat]. Π[F, nat]. ptr(T, a)"), {"slot"});

    cn_br_      = axiom(parse(*this, "cn[bool, cn[], cn[]]"), {"br"});
    cn_pe_info_ = axiom(parse(*this, "cn[T: *, ptr(int {8s64: nat}, {0s64: nat}), T]"), {"pe_info"});
    cn_match_   = axiom(parse(*this, "cn[T: *, a: 𝔸, [a; [T, cn[]]]]"), {"match"});
    cn_end_     = axiom(parse(*this, "cn[]"), {"end"});

#define CODE(T, o) op<T::o>()->set_normalizer(normalize_ ## o);
    THORIN_W_OP (CODE)
    THORIN_M_OP (CODE)
    THORIN_I_OP (CODE)
    THORIN_R_OP (CODE)
#undef CODE

#define CODE(T, o) op<T::o>()->set_normalizer(normalize_ ## T<T::o>);
    THORIN_I_CMP(CODE)
    THORIN_R_CMP(CODE)
#undef CODE
}

const Def* World::op_enter(const Def* mem, Debug dbg) {
    return app(op_enter_, mem, dbg);
}

const Def* types_from_tuple_type(World& w, const Def* type) {
    if (auto sig = type->isa<Sigma>()) {
        return w.tuple(sig->ops());
    } else if (auto var = type->isa<Variadic>()) {
        return w.pack(var->arity(w), var->body());
    }
    return type;
}

const Def* World::op_lea(const Def* ptr, const Def* index, Debug dbg) {
    auto types = types_from_tuple_type(*this, app_arg(*this, ptr->type(), 0));
    return app(app(op_lea_, {types->arity(*this), types, app_arg(*this, ptr->type(), 1)}, dbg), {ptr, index}, dbg);
}

const Def* World::op_lea(const Def* ptr, size_t i, Debug dbg) {
    auto types = types_from_tuple_type(*this, app_arg(*this, ptr->type(), 0));
    auto idx = index(types->arity(*this)->as<Arity>()->value(), i);
    return app(app(op_lea_, {types->arity(*this), types, app_arg(*this, ptr->type(), 1)}, dbg), {ptr, idx}, dbg);
}

const Def* World::op_load(const Def* mem, const Def* ptr, Debug dbg) {
    return app(app(op_load_, ptr->type()->as<App>()->arg(), dbg), {mem, ptr}, dbg);
}

const Def* World::op_slot(const Def* type, const Def* frame, Debug dbg) {
    return app(app(op_slot_, {type, lit_nat_0()}, dbg), {frame, lit_nat(Def::gid_counter())}, dbg);
}

const Def* World::op_store(const Def* mem, const Def* ptr, const Def* val, Debug dbg) {
    return app(app(op_store_, ptr->type()->as<App>()->arg(), dbg), {mem, ptr, val}, dbg);
}

}
