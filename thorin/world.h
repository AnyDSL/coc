#ifndef THORIN_WORLD_H
#define THORIN_WORLD_H

#include <memory>
#include <string>

#include "thorin/def.h"
#include "thorin/tables.h"

namespace thorin {

class WorldBase {
public:
    struct DefHash {
        static uint64_t hash(const Def* def) { return def->hash(); }
        static bool eq(const Def* d1, const Def* d2) { return d2->equal(d1); }
        static const Def* sentinel() { return (const Def*)(1); }
    };

    typedef HashSet<const Def*, DefHash> DefSet;

    WorldBase& operator=(const WorldBase&) = delete;
    WorldBase(const WorldBase&) = delete;

    WorldBase();
    ~WorldBase();

    const Universe* universe() const { return universe_; }
    const Star* star(Qualifier q = Qualifier::Unlimited) const { return star_[size_t(q)]; }
    const Star* star(const Def* q) {
        if (auto cq = isa_const_qualifier(q))
            return star(cq->box().get_qualifier());
        return unify<Star>(1, *this, q);
    }

    const Axiom* arity_kind() const { return arity_kind_; }
    const Axiom* arity(size_t a, Debug dbg = {}) { return assume(arity_kind(), {u64(a)}, dbg); }

    const Axiom* qualifier_kind() const { return qualifier_kind_; }
    const Axiom* qualifier(Qualifier q = Qualifier::Unlimited) const { return qualifier_[size_t(q)]; }
    const Axiom* unlimited() const { return qualifier(Qualifier::Unlimited); }
    const Axiom* affine() const { return qualifier(Qualifier::Affine); }
    const Axiom* linear() const { return qualifier(Qualifier::Linear); }
    const Axiom* relevant() const { return qualifier(Qualifier::Relevant); }
    bool is_qualifier(const Def* def) const {
        return def->type() == qualifier_kind();
    }
    const Axiom* isa_const_qualifier(const Def* def) const {
        assert(def != nullptr);
        for (auto q : qualifier_)
            if (q == def) return q;
        return nullptr;
    }

    const Def* index(size_t index, size_t arity, Debug dbg = {});
    const Def* variadic(const Def* arity, const Def* body, Debug dbg = {});
    /// @em nominal Axiom
    const Axiom* axiom(const Def* type, Debug dbg = {}) { return insert<Axiom>(0, *this, type, dbg); }
    /// @em structural Axiom
    const Axiom* assume(const Def* type, Box box, Debug dbg = {}) {
        return unify<Axiom>(0, *this, type, box, dbg);
    }
    const Def* dim(const Def* def, Debug dbg = {});
    const Error* error(const Def* type) { return unify<Error>(0, *this, type); }
    const Var* var(Defs types, size_t index, Debug dbg = {}) { return var(sigma(types), index, dbg); }
    const Var* var(const Def* type, size_t index, Debug dbg = {}) {
        return unify<Var>(0, *this, type, index, dbg);
    }

    const Pi* pi(const Def* domain, const Def* body, Debug dbg = {}) {
        return pi(Defs({domain}), body, dbg);
    }
    const Pi* pi(Defs domains, const Def* body, Debug dbg = {}) {
        return pi(domains, body, unlimited(), dbg);
    }
    const Pi* pi(const Def* domain, const Def* body, const Def* qualifier, Debug dbg = {}) {
        return pi(Defs({domain}), body, qualifier, dbg);
    }
    const Pi* pi(Defs domains, const Def* body, const Def* qualifier, Debug dbg = {});
    const Lambda* lambda(const Def* domain, const Def* body, Debug dbg = {}) {
        return lambda(domain, body, unlimited(), dbg);
    }
    const Lambda* lambda(const Def* domain, const Def* body, const Def* type_qualifier,
                         Debug dbg = {}) {
        return lambda(Defs({domain}), body, type_qualifier, dbg);
    }
    const Lambda* lambda(Defs domains, const Def* body, Debug dbg = {}) {
        return lambda(domains, body, unlimited(), dbg);
    }
    const Lambda* lambda(Defs domains, const Def* body, const Def* type_qualifier, Debug dbg = {}) {
        return pi_lambda(pi(domains, body->type(), type_qualifier), body, dbg);
    }
    Lambda* pi_lambda(const Pi* pi, Debug dbg = {}) {
        return insert<Lambda>(1, *this, pi, dbg);
    }
    const Lambda* pi_lambda(const Pi* pi, const Def* body, Debug dbg = {});
    const Def* app(const Def* callee, Defs args, Debug dbg = {});
    const Def* app(const Def* callee, const Def* arg, Debug dbg = {}) {
        return app(callee, Defs({arg}), dbg);
    }
    const Def* app(const Def* callee, Debug dbg = {}) {
        return app(callee, tuple0(), dbg);
    }

    const Sigma* unit(Qualifier q = Qualifier::Unlimited) { return unit_[size_t(q)]; }
    const Sigma* unit(const Def* q) {
        if (auto cq = isa_const_qualifier(q))
            return unit(cq->box().get_qualifier());
        return unify<Sigma>(0, *this, Defs(), star(q), Debug("Σ()"));
    }
    /// Structural sigma types or kinds
    const Def* sigma(Defs defs, Debug dbg = {}) { return sigma(defs, nullptr, dbg); }
    const Def* sigma(Defs, const Def* qualifier, Debug dbg = {});
    /// Nominal sigma types or kinds
    Sigma* sigma(size_t num_ops, const Def* type, Debug dbg = {}) {
        return insert<Sigma>(num_ops, *this, type, num_ops, dbg);
    }
    /// Nominal sigma types
    Sigma* sigma_type(size_t num_ops, Debug dbg = {}) {
        return sigma_type(num_ops, unlimited(), dbg);
    }
    Sigma* sigma_type(size_t num_ops, const Def* qualifier, Debug dbg = {}) {
        return sigma(num_ops, star(qualifier), dbg);
    }
    /// Nominal sigma kinds
    Sigma* sigma_kind(size_t num_ops, Debug dbg = {}) {
        return insert<Sigma>(num_ops, *this, num_ops, dbg);
    }

    const Tuple* tuple0(Qualifier q = Qualifier::Unlimited) { return tuple0_[size_t(q)]; }
    const Tuple* tuple0(const Def* q) {
        if (auto cq = isa_const_qualifier(q))
            return tuple0(cq->box().get_qualifier());
        return unify<Tuple>(0, *this, unit(q), Defs(), Debug("()"));
    }
    const Def* tuple(Defs defs, Debug dbg = {}) {
        return tuple(sigma(types(defs), dbg), defs, dbg);
    }
    const Def* tuple(Defs defs, const Def* type_q, Debug dbg = {}) {
        return tuple(sigma(types(defs), type_q, dbg), defs, dbg);
    }
    const Def* tuple(const Def* type, Defs defs, Debug dbg = {});
    const Def* extract(const Def* def, const Def* index, Debug dbg = {});
    const Def* extract(const Def* def, size_t index, Debug dbg = {});

    const Def* intersection(Defs defs, Debug dbg = {});
    const Def* intersection(Defs defs, const Def* type, Debug dbg = {});
    const Def* pick(const Def* type, const Def* def, Debug dbg = {});

    const Def* variant(Defs defs, Debug dbg = {});
    const Def* variant(Defs defs, const Def* type, Debug dbg = {});
    Variant* variant(size_t num_ops, const Def* type, Debug dbg = {}) {
        assert(num_ops > 1 && "It should not be necessary to build empty/unary variants.");
        return insert<Variant>(num_ops, *this, type, num_ops, dbg);
    }
    const Def* any(const Def* type, const Def* def, Debug dbg = {});
    const Def* match(const Def* def, Defs handlers, Debug dbg = {});

    const Def* singleton(const Def* def, Debug dbg = {});

    const DefSet& defs() const { return defs_; }

    friend void swap(WorldBase& w1, WorldBase& w2) {
        using std::swap;
        swap(w1.defs_, w2.defs_);
        w1.fix();
        w2.fix();
    }

private:
    void fix() { for (auto def : defs_) def->world_ = this; }

protected:
    template<class T, class... Args>
    const T* unify(size_t num_ops, Args&&... args) {
        auto def = alloc<T>(num_ops, args...);
        assert(!def->is_nominal());
        auto p = defs_.emplace(def);
        if (p.second) {
            def->wire_uses();
            return def;
        }

        --Def::gid_counter_;
        dealloc(def);
        return static_cast<const T*>(*p.first);
    }

    template<class T, class... Args>
    T* insert(size_t num_ops, Args&&... args) {
        auto def = alloc<T>(num_ops, args...);
        auto p = defs_.emplace(def);
        assert_unused(p.second);
        return def;
    }

    struct Page {
        static const size_t Size = 1024 * 1024 - sizeof(std::unique_ptr<int>); // 1MB - sizeof(next)
        std::unique_ptr<Page> next;
        char buffer[Size];
    };

    static bool alloc_guard_;

    template<class T, class... Args>
    T* alloc(size_t num_ops, Args&&... args) {
        static_assert(sizeof(Def) == sizeof(T), "you are not allowed to introduce any additional data in subclasses of Def");
#ifndef NDEBUG
        assert(alloc_guard_ = !alloc_guard_ && "you are not allowed to recursively invoke alloc");
#endif
        size_t num_bytes = sizeof(T) + sizeof(const Def*) * num_ops;
        assert(num_bytes < Page::Size);

        if (buffer_index_ + num_bytes >= Page::Size) {
            auto page = new Page;
            cur_page_->next.reset(page);
            cur_page_ = page;
            buffer_index_ = 0;
        }

        auto result = new (cur_page_->buffer + buffer_index_) T(args...);
        buffer_index_ += num_bytes;
        assert(buffer_index_ % alignof(T) == 0);

#ifndef NDEBUG
        alloc_guard_ = !alloc_guard_;
#endif
        return result;
    }

    template<class T>
    void dealloc(const T* def) {
        size_t num_bytes = sizeof(T) + def->num_ops() * sizeof(const Def*);
        def->~T();
        if (ptrdiff_t(buffer_index_ - num_bytes) > 0) // don't care otherwise
            buffer_index_-= num_bytes;
        assert(buffer_index_ % alignof(T) == 0);
    }

    std::unique_ptr<Page> root_page_;
    Page* cur_page_;
    size_t buffer_index_ = 0;
    DefSet defs_;
    const Universe* universe_;
    const Axiom* qualifier_kind_;
    std::array<const Axiom*, 4> qualifier_;
    std::array<const Star*, 4> star_;
    std::array<const Sigma*, 4> unit_;
    std::array<const Tuple*, 4> tuple0_;
    const Axiom* arity_kind_;
};

class World : public WorldBase {
public:
    World();

    //@{ types and type constructors
    const Def* type_bool(const Def* q = nullptr) {
        if (q == nullptr || q == unlimited())
            return type_bool_;
        return app(type_bool_q_, q);
    }
    const Def* type_nat(const Def* q = nullptr) {
        if (q == nullptr || q == unlimited())
            return type_nat_;
        return app(type_nat_q_, q);
    }

    const Axiom* type_int() { return type_int_; }
    const App* type_int(Qualifier q, ITypeFlags flags, int64_t width) {
        return type_int_qfw_[size_t(q)][size_t(flags)][iwidth2index(width)];
    }
    const App* type_int(const Def* q, const Def* flags, const Def* width, Debug dbg = {}) {
        return app(type_int_, {q, flags, width}, dbg)->as<App>();
    }

    const Axiom* type_real() { return type_real_; }
    const App* type_real(Qualifier q, RTypeFlags flags, int64_t width) {
        return type_real_qfw_[size_t(q)][size_t(flags)][rwidth2index(width)];
    }
    const App* type_real(const Def* q, const Def* flags, const Def* width, Debug dbg = {}) {
        return app(type_real_, {q, flags, width}, dbg)->as<App>();
    }

#define CODE(r, prod) \
    const App* BOOST_PP_CAT(type_, BOOST_PP_SEQ_CAT(prod)) () { return BOOST_PP_CAT(BOOST_PP_CAT(type_, BOOST_PP_SEQ_CAT(prod)), _); }
    BOOST_PP_SEQ_FOR_EACH_PRODUCT(CODE, (THORIN_Q)(THORIN_I_FLAGS)(THORIN_I_WIDTH))
    BOOST_PP_SEQ_FOR_EACH_PRODUCT(CODE, (THORIN_Q)(THORIN_R_FLAGS)(THORIN_R_WIDTH))
#undef CODE

    const Axiom* type_mem() { return type_mem_; }
    const Axiom* type_frame() { return type_frame_; }
    const Axiom* type_ptr() { return type_ptr_; }
    const Def* type_ptr(const Def* pointee, Debug dbg = {}) { return type_ptr(pointee, val_nat_0(), dbg); }
    const Def* type_ptr(const Def* pointee, const Def* addr_space, Debug dbg = {}) {
        return app(type_ptr_, {pointee, addr_space}, dbg);
    }
    //@}

    //@{ values
    const Axiom* val_nat(int64_t val, const Def* q = nullptr) {
        return assume(type_nat(q), {val}, {std::to_string(val)});
    }
    const Axiom* val_nat_0() { return val_nat_0_; }
    const Axiom* val_nat_1() { return val_nat_[0]; }
    const Axiom* val_nat_2() { return val_nat_[1]; }
    const Axiom* val_nat_4() { return val_nat_[2]; }
    const Axiom* val_nat_8() { return val_nat_[3]; }
    const Axiom* val_nat_16() { return val_nat_[4]; }
    const Axiom* val_nat_32() { return val_nat_[5]; }
    const Axiom* val_nat_64() { return val_nat_[6]; }

    const Axiom* val_bool(bool val) { return val_bool_[size_t(val)]; }
    const Axiom* val_bool_bot() { return val_bool_[0]; }
    const Axiom* val_bool_top() { return val_bool_[1]; }

#define CODE(r, prod) \
    const Axiom* BOOST_PP_CAT(val_, BOOST_PP_SEQ_CAT(prod)) (BOOST_PP_SEQ_CAT(BOOST_PP_SEQ_TAIL(prod)) val) { \
        return assume(BOOST_PP_CAT(type_, BOOST_PP_SEQ_CAT(prod))(), {val}, {std::to_string(val)}); \
    }
    BOOST_PP_SEQ_FOR_EACH_PRODUCT(CODE, (THORIN_Q)(THORIN_I_FLAGS)(THORIN_I_WIDTH))
    BOOST_PP_SEQ_FOR_EACH_PRODUCT(CODE, (THORIN_Q)(THORIN_R_FLAGS)(THORIN_R_WIDTH))
#undef CODE
    //@}

    //@{ arithmetic operations
#define CODE(x) \
    const Axiom* op_ ## x() { return op_ ## x ## _; } \
    const App* op_ ## x(const Def*, const Def*) { return nullptr; }
    THORIN_I_ARITHOP(CODE)
    THORIN_R_ARITHOP(CODE)
#undef CODE
    //@}

    //@{ relational operations
    // TODO
    //@}

    //@{ tuple operations
    const Axiom* op_insert() { return op_insert_; }
    const Def* op_insert(const Def* def, const Def* index, const Def* val, Debug dbg = {});
    const Def* op_insert(const Def* def, size_t index, const Def* val, Debug dbg = {});
    const Axiom* op_lea() { return op_lea_; }
    const Def* op_lea(const Def* ptr, const Def* index, Debug dbg = {});
    const Def* op_lea(const Def* ptr, size_t index, Debug dbg = {});
    //@}

    //@{ memory operations
    // TODO
    //@}
private:
    const Axiom* type_nat_q_;
    const Def* type_nat_;
    const Axiom* val_nat_0_;
    std::array<const Axiom*, 7> val_nat_;
    const Axiom* type_bool_q_;
    const Def* type_bool_;
    std::array<const Axiom*, 2> val_bool_;
    const Axiom* type_int_;
    const Axiom* type_real_;
    const Axiom* type_mem_;
    const Axiom* type_frame_;
    const Axiom* type_ptr_;
    const Pi* type_icmpop_;
    const Pi* type_rcmpop_;
    const Pi* type_iarithop_;
    const Pi* type_rarithop_;
    const Axiom* op_lea_;
    const Axiom* op_insert_;

#define CODE(x) \
    const Axiom* op_ ## x ## _; \
    const App* op_ ## x ## s_[size_t(IType::Num)];
    THORIN_I_ARITHOP(CODE)
#undef CODE

#define CODE(x) \
    const Axiom* op_ ## x ## _; \
    const App* op_ ## x ## s_[size_t(IType::Num)];
    THORIN_R_ARITHOP(CODE)
#undef CODE

    union {
        struct {
#define CODE(r, prod) \
            const App* BOOST_PP_CAT(BOOST_PP_CAT(type_, BOOST_PP_SEQ_CAT(prod)), _);
            BOOST_PP_SEQ_FOR_EACH_PRODUCT(CODE, (THORIN_Q)(THORIN_I_FLAGS)(THORIN_I_WIDTH))
#undef CODE
        };
        const App* type_int_qfw_[4][size_t(ITypeFlags::Num)][BOOST_PP_SEQ_SIZE(THORIN_I_WIDTH)];
    };

    union {
        struct {
#define CODE(r, prod) \
            const App* BOOST_PP_CAT(BOOST_PP_CAT(type_, BOOST_PP_SEQ_CAT(prod)), _);
            BOOST_PP_SEQ_FOR_EACH_PRODUCT(CODE, (THORIN_Q)(THORIN_R_FLAGS)(THORIN_R_WIDTH))
#undef CODE
        };
        const App* type_real_qfw_[4][size_t(RTypeFlags::Num)][BOOST_PP_SEQ_SIZE(THORIN_R_WIDTH)];
    };

#if 0
    const Axiom* op_icmp_;
    union {
        struct {
#define CODE(x) \
            const App* op_ ## icmp_ ## x ## _;
            THORIN_I_REL(CODE)
#undef CODE
        };
        const App* op_icmps_[size_t(IRel::Num)];
    };

    const Axiom* op_rcmp_;
    union {
        struct {
#define CODE(x) \
            const App* op_ ## rcmp_ ## x ## _;
            THORIN_I_REL(CODE)
#undef CODE
        };
        const App* op_rcmps_[size_t(IRel::Num)];
    };
#endif
};

}

#endif

