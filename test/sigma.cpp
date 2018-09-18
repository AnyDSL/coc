#include "gtest/gtest.h"

#include "thorin/world.h"
#include "thorin/fe/parser.h"

using namespace thorin;
using namespace thorin::fe;

TEST(Sigma, Assign) {
    World w;
    auto sig = w.sigma({w.kind_star(), w.var(w.kind_star(), 0)})->as<Sigma>();
    EXPECT_TRUE(sig->is_dependent());
    EXPECT_TRUE(sig->assignable(w.tuple({w.type_nat(), w.lit_nat(42)})));
    EXPECT_FALSE(sig->assignable(w.tuple({w.type_nat(), w.lit_false()})));
}

TEST(Sigma, ExtractAndSingleton) {
    World w;
    auto n23 = w.lit_nat(23);
    auto n42 = w.lit_nat(42);
    auto NxN = w.sigma({w.type_nat(), w.type_nat()});

    auto fst = w.lambda(NxN, w.extract(w.var(NxN, 0), 0_u64));
    auto snd = w.lambda(NxN, w.extract(w.var(NxN, 0), 1));
    EXPECT_EQ(w.app(fst, {n23, n42}), w.lit_nat(23));
    EXPECT_EQ(w.app(snd, {n23, n42}), w.lit_nat(42));

    auto poly = w.axiom(w.pi(w.kind_star(), w.kind_star()), {"Poly"});
    auto sigma = w.sigma({w.kind_star(), w.app(poly, w.var(w.kind_star(), 0))}, {"sig"})->as<Sigma>();
    EXPECT_TRUE(sigma->is_dependent());
    auto sigma_val = w.axiom(sigma,{"val"});
    auto fst_sigma = w.extract(sigma_val, 0_s);
    EXPECT_EQ(fst_sigma->type(), w.kind_star());
    auto snd_sigma = w.extract(sigma_val, 1);
    snd_sigma->type()->dump();
    auto single_sigma = w.singleton(sigma_val);
    std::cout << single_sigma << ": " << single_sigma->type() << std::endl;
    auto single_pi = w.singleton(w.axiom(w.pi(w.kind_star(), w.kind_star()), {"pival"}));
    std::cout << single_pi << ": " << single_pi->type() << std::endl;
}

TEST(Tuple, TypeExtract) {
    World w;
    auto arity2 = w.lit_arity(2);
    auto nat = w.axiom(w.kind_star(), {"Nat"});

    auto tup = w.tuple({arity2, nat});
    auto ex_tup = w.extract(tup, w.var(arity2, 0));
    EXPECT_EQ(ex_tup->type(), w.kind_star());

    auto ex_var_tup = w.extract(w.var(w.sigma({w.kind_arity(), w.kind_star()}), 1), w.var(arity2, 0))->type();
    EXPECT_EQ(ex_var_tup, w.kind_star());

    auto sig = w.sigma({w.pi(w.kind_star(), w.kind_star()), w.kind_star()});
    EXPECT_THROW(w.extract(w.var(sig, 1), w.var(arity2, 0))->type(), TypeError);
}

TEST(Tuple, TypeExtractFreeVars) {
    World w;

    EXPECT_EQ(parse(w, "\\0::[x:nat, y:nat, \\4::*]#2₃")->type(), w.var(w.kind_star(), 2));
}

TEST(Sigma, EtaConversion) {
    World w;
    auto v = w.lit_nat_32();
    auto N = w.type_nat();
    auto B = w.type_bool();

    EXPECT_EQ(w.extract(w.tuple({v, v, v}), w.var(w.lit_arity(3), 17)), v);
    EXPECT_EQ(parse(w, "‹\\42::𝔸 ; \\23::«\\43::𝔸; nat»#\\0::\\43::𝔸›"), parse(w, "\\22::«\\42::𝔸; nat»"));

    auto t = w.axiom(w.sigma({N, B}), {"t"});
    EXPECT_EQ(w.tuple({w.extract(t, 0_s), w.extract(t, 1_s)}), t);
}

// TODO add a test for passing around dependent sigma types, e.g. like the following
// TEST(Parser, NestedDependentBinders) {
//     WorldBase w;
//     auto S = w.kind_star();
//     auto N = w.axiom(S, {"nat"});
//     auto dtyp = w.axiom(w.pi(N, S), {"dt"});
//     auto npair = w.sigma({N, N});
//     auto sig = w.sigma({npair, w.app(dtyp, w.extract(w.var(npair, 0), (size_t)1))});
//     auto typ = w.axiom(w.pi(w.sigma({N, w.app(dtyp, w.var(N, 0))}), S), {"typ"});
//     Env env;
//     env["nat"] = N;
//     env["dt"] = dtyp;
//     env["typ"] = typ;
//     // passing d : dt(..) to typ(n1, d) is broken, as the variables in the type of d don't refer to n1 in the body of the pi and thus the app of typ doesn't type... do we need casting/ascribing here?
//     auto def = w.pi(sig, w.app(typ, w.tuple({w.extract(w.extract(w.var(sig, 0), (size_t)0), (size_t)1),
//                         w.extract(w.var(sig, 0), (size_t)1)})));
//     EXPECT_EQ(parse(w, "Π[[n0 : nat, n1: nat], d: dt(n1)]. typ(n1, d)", env), def);
// }
