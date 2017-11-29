#include "gtest/gtest.h"

#include <sstream>
#include <string>

#include "thorin/world.h"
#include "thorin/frontend/parser.h"

using namespace thorin;

TEST(Parser, SimplePi) {
    World w;
    auto def = w.pi(w.star(), w.pi(w.var(w.star(), 0), w.var(w.star(), 1)));
    ASSERT_EQ(parse(w, "Π(T:*). Π(U:T). T"), def);
}

TEST(Parser, SimpleLambda) {
    World w;
    auto def = w.lambda(w.star(), w.lambda(w.var(w.star(), 0), w.var(w.var(w.star(), 1), 0)));
    ASSERT_EQ(parse(w, "λ(T:*). λ(x:T). x"), def);
}

TEST(Parser, SigmaVariadic) {
    World w;

    // TODO this is broken: the lexer thinks this is EOF
    //ASSERT_EQ(parse(w, "[]"), w.unit());

    auto s = w.sigma({w.star(), w.var(w.star(), 0)});
    ASSERT_EQ(parse(w, "[T:*, T]"), s);

    auto v = w.variadic(w.multi_arity_kind(), w.var(w.multi_arity_kind(), 0));
    ASSERT_EQ(parse(w, "[x:𝕄;  x]"), v);
}
