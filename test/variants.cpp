#include "gtest/gtest.h"

#include "thorin/world.h"

using namespace thorin;

TEST(Variants, negative_test) {
    World w;
    auto boolean = w.boolean();
    auto nat = w.nat();
    auto variant = w.variant({nat, boolean});
    auto any_nat23 = w.any(variant, w.nat(23));
    auto handle_nat = w.lambda(nat, w.var(nat, 0));
    auto handle_bool = w.lambda(boolean, w.nat(0));
    auto handle_bool_bool = w.lambda({boolean, boolean}, w.nat(0));
    ASSERT_DEATH(w.match(any_nat23, {handle_bool_bool, handle_nat}), ".*");
}

TEST(Variants, positive_tests) {
    World w;
    auto boolean = w.boolean();
    auto nat = w.nat();
    // Test variant types and matches
    auto variant = w.variant({nat, boolean});
    auto any_nat23 = w.any(variant, w.nat(23));
    auto any_bool = w.any(variant, w.axiom(boolean,{"false"}));
    auto assumed_var = w.axiom(variant,{"someval"});
    auto handle_nat = w.lambda(nat, w.var(nat, 0));
    auto handle_bool = w.lambda(boolean, w.axiom(nat,{"0"}));
    Defs handlers{handle_nat, handle_bool};
    auto match_nat = w.match(any_nat23, handlers);
    match_nat->dump(); // 23
    auto o_match_nat = w.match(any_nat23, {handle_bool, handle_nat});
    assert(match_nat == o_match_nat);
    auto match_bool = w.match(any_bool, handlers);
    match_bool->dump(); // 0
    auto match = w.match(assumed_var, handlers);
    match->dump(); // match someval with ...
    match->type()->dump();
    // TODO don't want to allow this, does not have a real intersection interpretation, should be empty
    w.intersection({w.pi(nat, nat), w.pi(boolean, boolean)})->dump();
}
