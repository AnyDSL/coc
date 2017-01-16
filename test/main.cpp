#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Simple, unit) {
    World w;
    auto unitt = w.unit();
    auto unitv = w.unit_val();
    ASSERT_EQ(unitv->type(), unitt);

    auto lam = w.lambda(unitt, unitv);
    ASSERT_EQ(lam->domain(), unitt);
    ASSERT_TRUE(lam->domains().size() == 0);
    ASSERT_EQ(lam->type()->body(), unitt);
    auto pi = w.pi(Defs({}), unitt);
    ASSERT_EQ(pi, lam->type());
    auto apped = w.app(lam, unitv);
    ASSERT_EQ(unitv, apped);
}

TEST(Simple, Misc) {
    World w;
    auto n16 = w.val_nat16();
    n16->dump();
    ASSERT_EQ(n16, w.val_nat(16));

    auto n23 = w.val_nat(23);
    auto n23x = w.val_nat(23);
    ASSERT_EQ(n23, n23x);
    auto n42 = w.val_nat(42);
    /*auto n32 =*/ w.val_nat(32);
    auto Top = w.val_top();
    Top->dump();

    {
        auto s32w = w.type_int(32, ITypeFlags::sw);
        auto a = w.axiom(s32w, {"a"});
        auto b = w.axiom(s32w, {"b"});
        a->dump();
        b->dump();
        //auto add = w.app(w.app(w.iadd(), {n32, w.nat(3)}), {a, b});
        //auto mul = w.app(w.app(w.imul(), {n32, w.nat(3)}), {a, b});
        //add->dump();
        //add->type()->dump();
        //mul->dump();
        //mul->type()->dump();
        w.type_mem()->dump();

        auto load = w.axiom(w.pi(w.star(), w.pi({w.type_mem(), w.type_ptr(w.var(w.star(), 1))}, w.sigma({w.type_mem(), w.var(w.star(), 3)}))), {"load"});
        load->type()->dump();
        auto x = w.axiom(w.type_ptr(w.type_nat()), {"x"});
        auto m = w.axiom(w.type_mem(), {"m"});
        auto nat_load = w.app(load, w.type_nat());
        nat_load->dump();
        nat_load->type()->dump();
        auto p = w.app(nat_load, {m, x});
        p->dump();
        p->type()->dump();
    }


    // λT:*.λx:T.x
    auto T_1 = w.var(w.star(), 0, {"T"});
    ASSERT_TRUE(T_1->free_vars().test(0));
    ASSERT_FALSE(T_1->free_vars().test(1));
    auto T_2 = w.var(w.star(), 1, {"T"});
    ASSERT_TRUE(T_2->free_vars().test(1));
    ASSERT_TRUE(T_2->free_vars().any_begin(1));
    auto x = w.var(T_2, 0, {"x"});
    auto poly_id = w.lambda(T_2->type(), w.lambda(T_1, x));
    ASSERT_FALSE(poly_id->free_vars().test(0));
    ASSERT_FALSE(poly_id->free_vars().test(1));
    ASSERT_FALSE(poly_id->free_vars().any_end(2));
    ASSERT_FALSE(poly_id->free_vars().any_begin(0));
    poly_id->dump();
    poly_id->type()->dump();

    // λx:w.type_nat().x
    auto int_id = w.app(poly_id, w.type_nat());
    int_id->dump();
    int_id->type()->dump();

    auto fst = w.lambda({w.type_nat(), w.type_nat()}, w.var(w.type_nat(), 1));
    auto snd = w.lambda({w.type_nat(), w.type_nat()}, w.var(w.type_nat(), 0));
    w.app(fst, {n23, n42})->dump(); // 23
    w.app(snd, {n23, n42})->dump(); // 42


    //// 2
    //w.app(int_id, n2)->dump();

    //// 3
    //w.extract(w.tuple({n1, n2, n3}), 2)->dump();
    ////w.extract(n3, 0)->dump();

    //auto make_pair = w.axiom(w.pi(w.unit(), w.sigma({w.type_nat(), w.type_nat()})), {"make_pair"});
    //make_pair->dump();
    //w.app(make_pair, n1)->dump();

    //auto plus = w.axiom(w.pi({w.type_nat(), w.type_nat()}, w.type_nat()), {"+"});
    //plus->type()->dump();
    //w.app(int_id, w.app(plus, {w.app(plus, {n1, n2}), n3}))->dump();

    auto Arr = w.axiom(w.pi({w.type_nat(), w.pi(w.type_nat(), w.star())}, w.star()),{"Arr"});
    Defs dom{w.type_nat(), w.star()};
    auto _Arr = w.lambda(dom, w.app(Arr, {w.var(w.type_nat(), 1), w.lambda(w.type_nat(), w.var(w.star(), 1))}));
    _Arr->dump();

    auto arr = w.app(_Arr, {n23, w.type_nat()});
    arr->dump();

    // Test projections from dependent sigmas
    auto poly = w.axiom(w.pi(w.star(), w.star()),{"Poly"});
    auto sigma = w.sigma({w.star(), w.app(poly, w.var(w.star(), 0))}, {"sig"});
    auto sigma_val = w.axiom(sigma,{"val"});
    std::cout << sigma_val << ": " << sigma << endl;
    auto fst_sigma = w.extract(sigma_val, 0);
    std::cout << fst_sigma << ": " << fst_sigma->type() << endl;
    auto snd_sigma = w.extract(sigma_val, 1);
    std::cout << snd_sigma << ": " << snd_sigma->type() << endl;


    // Test singleton types and kinds
    auto single_sigma = w.singleton(sigma_val);
    std::cout << single_sigma << ": " << single_sigma->type() << std::endl;
    auto single_pi = w.singleton(w.axiom(w.pi(w.star(), w.star()), {"pival"}));
    std::cout << single_pi << ": " << single_pi->type() << std::endl;
}

int main(int argc, char** argv)  {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
