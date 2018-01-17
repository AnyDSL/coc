#include "gtest/gtest.h"

#include <sstream>
#include <string>

#include "thorin/world.h"
#include "thorin/frontend/parser.h"

using namespace thorin;

TEST(Parser, SimplePi) {
    World w;
    auto def = w.pi(w.star(), w.pi(w.var(w.star(), 0), w.var(w.star(), 1)));
    ASSERT_EQ(parse(w, "ΠT:*. ΠU:T. T"), def);
}

TEST(Parser, SimpleLambda) {
    World w;
    auto def = w.lambda(w.star(), w.lambda(w.var(w.star(), 0), w.var(w.var(w.star(), 1), 0)));
    ASSERT_EQ(parse(w, "λT:*. λx:T. x"), def);
}

TEST(Parser, SimpleSigma) {
    World w;

    ASSERT_EQ(parse(w, "[]"), w.unit());

    auto s = w.sigma({w.star(), w.var(w.star(), 0)});
    ASSERT_EQ(parse(w, "[T:*, T]"), s);
}

TEST(Parser, SimpleVariadic) {
    World w;
    auto S = w.star();
    auto M = w.multi_arity_kind();

    // TODO simplify further once we can parse arity literals
    auto v = w.pi(M, w.pi(w.variadic(w.var(M, 0), S), S));
    ASSERT_EQ(parse(w, "Πa:𝕄. Πx:[a; *]. *"), v);
}

TEST(Parser, Star) {
    World w;
    EXPECT_EQ(parse(w, "*"), w.star());
    EXPECT_EQ(parse(w, "*ᵁ"), w.star());
    EXPECT_EQ(parse(w, "*ᴿ"), w.star(QualifierTag::Relevant));
    EXPECT_EQ(parse(w, "*ᴬ"), w.star(QualifierTag::Affine));
    EXPECT_EQ(parse(w, "*ᴸ"), w.star(QualifierTag::Linear));
    EXPECT_EQ(parse(w, "Πq:ℚ.*q"), w.pi(w.qualifier_type(), w.star(w.var(w.qualifier_type(), 0))));
}

TEST(Parser, ComplexVariadics) {
    World w;
    auto S = w.star();
    auto M = w.multi_arity_kind();

    auto v = w.pi(M, w.pi(w.variadic(w.var(M, 0), S),
                          w.variadic(w.var(M, 1),
                                     w.extract(w.var(w.variadic(w.var(M, 2), S), 1), w.var(w.var(M, 2), 0)))));
    ASSERT_EQ(parse(w, "Πa:𝕄. Πx:[a; *]. [i:a; x#i]"), v);
}

TEST(Parser, NestedBinders) {
    World w;
    auto S = w.star();
    auto N = w.axiom(S, {"nat"});
    auto sig = w.sigma({N, N});
    auto typ = w.axiom(w.pi(sig, S), {"typ"});
    auto typ2 = w.axiom(w.pi(sig, S), {"typ2"});
    Env env;
    env["nat"] = N;
    env["typ"] = typ;
    env["typ2"] = typ2;
    auto def = w.pi(sig, w.pi(w.app(typ, w.tuple({w.extract(w.var(sig, 0), 1), w.extract(w.var(sig, 0), 0)})),
                              w.app(typ2, w.var(sig, 1))));
    ASSERT_EQ(parse(w, "Πp:[n: nat, m: nat]. Πtyp(m, n). typ2(p)", env), def);
}

TEST(Parser, NestedBinders2) {
    World w;
    auto S = w.star();
    auto N = w.axiom(S, {"nat"});
    auto sig = w.sigma({N, w.sigma({N, N})});
    auto typ = w.axiom(w.pi(w.sigma({N, N, w.sigma({N, N})}), S), {"typ"});
    Env env;
    env["nat"] = N;
    env["typ"] = typ;
    auto def = w.pi(sig, w.app(typ, w.tuple({w.extract(w.extract(w.var(sig, 0), 1), 1),
                                             w.extract(w.extract(w.var(sig, 0), 1), 0),
                                             w.extract(w.var(sig, 0), 1)})));
    ASSERT_EQ(parse(w, "Π[m: nat, n: [n0 : nat, n1: nat]]. typ(n1, n0, n)", env), def);
}

TEST(Parser, NestedDependentBinders) {
    World w;
    auto S = w.star();
    auto N = w.axiom(S, {"nat"});
    auto dtyp = w.axiom(w.pi(N, S), {"dt"});
    auto npair = w.sigma({N, N});
    auto sig = w.sigma({npair, w.app(dtyp, w.extract(w.var(npair, 0), 1))});
    auto typ = w.axiom(w.pi(w.sigma({N, w.app(dtyp, w.var(N, 0))}), S), {"typ"});
    Env env;
    env["nat"] = N;
    env["dt"] = dtyp;
    env["typ"] = typ;
    auto def = w.pi(sig, w.app(typ, w.tuple({w.extract(w.extract(w.var(sig, 0), 0), 1),
                                             w.extract(w.var(sig, 0), 1)})));
    ASSERT_EQ(parse(w, "Π[[n0 : nat, n1: nat], d: dt(n1)]. typ(n1, d)", env), def);
}

TEST(Parser, IntArithOp) {
    World w;
    auto Q = w.qualifier_type();
    auto S = w.star();
    auto N = w.axiom(S, {"nat" });
    auto MA = w.multi_arity_kind();
    auto sig = w.sigma({Q, N, N});
    auto type_i = w.axiom(w.pi(sig, w.star(w.extract(w.var(sig, 0), 0))), {"int"});
    Env env;
    env["nat"]  = N;
    env["int"]  = type_i;
    auto dom = w.sigma({w.app(type_i, w.var(sig, 0)), w.app(type_i, w.var(sig, 1))});
    auto def = w.pi(MA, w.pi(sig, w.pi(dom, w.app(type_i, w.var(sig, 1)))));
    auto i_arithop = parse(w, "Πs: 𝕄. Π[q: ℚ, f: nat, w: nat]. Π[int(q, f, w),  int(q, f, w)].  int(q, f, w)", env);
    ASSERT_EQ(i_arithop, def);
}
