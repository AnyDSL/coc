#ifndef THORIN_REDUCE_H
#define THORIN_REDUCE_H

#include "thorin/def.h"

namespace thorin {

/// Reduces @p def with @p args using @p index to indicate the Var.
const Def* reduce(World&, const Def* def, Defs args, size_t index = 0);

/**
 * Reduces @p def with @p args using @p index to indicate the Var.
 * First, all structural stuff in @p def is reduced.
 * Then, the hook @p f is called with the result of the first reduction run.
 * Finally, a fixed-point iteration is performed to reduce the rest.
 */
const Def* reduce(World&, const Def* def, Defs args, std::function<void(const Def*)> f, size_t index = 0);

/// Flattens the use of Var @c 0 and Extract%s in @p body to directly use @p args instead.
const Def* flatten(World&, const Def* body, Defs args);

/// Unflattens the use of Vars @c 0, ..., n and uses Extract%s in @p body instead.
const Def* unflatten(World&, const Def* body, const Def* arg);

/// Adds @p shift to all free variables in @p def.
const Def* shift_free_vars(World&, const Def* def, int64_t shift);
}

#endif
