#include "thorin/world.h"
#include "utils/unicodemanager.h"

using namespace thorin;

void testQualifiers() {
    auto U = Qualifier::Unrestricted;
    auto A = Qualifier::Affine;
    auto R = Qualifier::Relevant;
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
    auto anx = w.var(ANat, 1, "x");
    auto anid = w.lambda(ANat, anx, "anid");
    auto app1 = w.app(anid, an0);
    assert(w.app(anid, an0) == w.error());

    auto tuple_type = w.sigma({w.nat(A), w.nat(R)});
    assert(tuple_type->qualifier() == L);
}
