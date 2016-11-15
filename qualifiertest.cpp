#include "thorin/world.h"
#include "utils/unicodemanager.h"

using namespace thorin;

#define printValue(x) do{ printUtf8(x->name()); printUtf8(" = "); x->dump(); }while(0)
#define printType(x) do{ printUtf8(x->name()); printUtf8(": "); x->type()->dump(); }while(0)

void testQualifiers() {
    auto U = Qualifier::Unrestricted;
    auto R = Qualifier::Relevant;
    auto A = Qualifier::Affine;
    auto L = Qualifier::Linear;

    assert(A < U);
    assert(R < U);
    assert(L < U);
    assert(L < A);
    assert(L < R);

    auto meet = [](Qualifier::URAL a, Qualifier::URAL b) { return Qualifier::meet(a, b); };
    assert(meet(U, U) == U);
    assert(meet(A, U) == A);
    assert(meet(R, U) == R);
    assert(meet(L, U) == L);
    assert(meet(A, A) == A);
    assert(meet(A, R) == L);
    assert(meet(L, A) == L);
    assert(meet(L, R) == L);

    World w;
    auto Nat = w.nat();
    auto ANat = w.nat(A);
    auto RNat = w.nat(R);
    auto LNat = w.nat(L);
    auto an0 = w.assume(ANat, "0");
    auto anx = w.var(ANat, 0, "x");
    auto anid = w.lambda(ANat, anx, "anid");
    auto app1 = w.app(anid, an0);
    assert(w.app(anid, an0) == w.error());

    auto tuple_type = w.sigma({ANat, RNat});
    assert(tuple_type->qualifier() == L);
    auto an1 = w.assume(ANat, "1");
    auto rn0 = w.assume(RNat, "0");
    auto tuple = w.tuple({an1, rn0});
    assert(tuple->type() == tuple_type);
    auto tuple_app0 = w.extract(tuple, 0);
    assert(w.extract(tuple, 0) == w.error());

    auto a_id_type = w.pi(Nat, Nat, A);
    auto nx = w.var(Nat, 0, "x");
    auto a_id = w.lambda(Nat, nx, A, "a_id");
    assert(a_id_type == a_id->type());
    auto n0 = w.assume(Nat, "0");
    auto a_id_app = w.app(a_id, n0);
    assert(w.app(a_id, n0) == w.error());

    // λᴬT:*.λx:ᴬT.x
    auto aT1 = w.var(w.star(A), 0, "T");
    auto aT2 = w.var(w.star(A), 1, "T");
    auto x = w.var(aT2, 0, "x");
    auto poly_aid = w.lambda(aT2->type(), w.lambda(aT1, x));
    cout << poly_aid << " : " << poly_aid->type() << endl;

    // λx:ᴬNat.x
    auto anid2 = w.app(poly_aid, ANat);
    cout << anid2 << " : " << anid2->type() << endl;

    cout << "--- Unrestricted Refs ---" << endl;
    {
        auto Ref = w.assume(w.pi(w.star(), w.star()), "Ref");
        printType(Ref);
        auto T_1 = w.var(w.star(), 0, "T");
        auto T_2 = w.var(w.star(), 1, "T");
        auto app_Ref_T_1 = w.app(Ref, T_1);
        auto NewRef = w.assume(w.pi(w.star(), w.pi(T_1, w.app(Ref, T_2))), "NewRef");
        printType(NewRef);
        auto ReadRef = w.assume(w.pi(w.star(), w.pi(w.app(Ref, T_1), T_2)), "ReadRef");
        printType(ReadRef);
        auto WriteRef = w.assume(w.pi(w.star(), w.pi({app_Ref_T_1, T_1}, w.unit())), "WriteRef");
        printType(WriteRef);
        auto FreeRef = w.assume(w.pi(w.star(), w.pi(app_Ref_T_1, w.unit())), "FreeRef");
        printType(FreeRef);
    }
    cout << "--- Affine Refs ---" << endl;
    {
        auto Ref = w.assume(w.pi(w.star(), w.star(A)), "ARef");
        printType(Ref);
        auto T_0 = w.var(w.star(), 0, "T");
        auto T_1 = w.var(w.star(), 1, "T");
        auto T_2 = w.var(w.star(), 2, "T");
        auto app_Ref_T_0 = w.app(Ref, T_0);
        auto NewRef = w.assume(w.pi(w.star(), w.pi(T_0, w.app(Ref, T_1))), "NewARef");
        printType(NewRef);
        //  Π(*).Π(ARef[<0:*>]).Σ(<1:*>, ARef[<2:*>])
        auto ReadRef = w.assume(w.pi(w.star(), w.pi(app_Ref_T_0, w.sigma({T_1, w.app(Ref, T_2)}))), "ReadARef");
        printType(ReadRef);
        auto WriteRef = w.assume(w.pi(w.star(), w.pi({app_Ref_T_0, T_0}, w.unit())), "WriteARef");
        printType(WriteRef);
        auto FreeRef = w.assume(w.pi(w.star(), w.pi(app_Ref_T_0, w.unit())), "FreeARef");
        printType(FreeRef);
    }
    cout << "--- Affine Capabilities for Refs ---" << endl;
    {
        auto Ref = w.assume(w.pi({w.star(), w.star()}, w.star()), "CRef");
        auto Cap = w.assume(w.pi(w.star(), w.star(A)), "ACap");
        printType(Ref);
        printType(Cap);
        auto T_1 = w.var(w.star(), 0, "T");
        auto C_1 = w.var(w.star(), 0, "C");
        auto T_2 = w.var(w.star(), 1, "T");
        auto C_2 = w.var(w.star(), 1, "C");
        auto T_3 = w.var(w.star(), 2, "T");
        auto app_Ref_T_1 = w.app(Ref, T_1);
        auto sigma = w.sigma({w.star(), w.app(Ref, {T_2, C_1}), w.app(Cap, C_2)});
        auto NewRef = w.assume(w.pi(w.star(), w.pi(T_1, sigma)), "NewCRef");
        printType(NewRef);
        auto ReadRef = w.assume(w.pi(w.star(), w.pi(app_Ref_T_1, w.sigma({T_2, app_Ref_T_1}))), "ReadCRef");
        printType(ReadRef);
        auto WriteRef = w.assume(w.pi(w.star(), w.pi({app_Ref_T_1, T_1}, w.unit())), "WriteCRef");
        printType(WriteRef);
        auto FreeRef = w.assume(w.pi(w.star(), w.pi(app_Ref_T_1, w.unit())), "FreeCRef");
        printType(FreeRef);
    }
    cout << "--- QualifierTest end ---" << endl;
}
