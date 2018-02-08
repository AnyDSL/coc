#ifndef THORIN_CORE_NORMALIZE_H
#define THORIN_CORE_NORMALIZE_H

#include "thorin/core/tables.h"
#include "thorin/util/location.h"

namespace thorin {

class Def;

namespace core {

#define CODE(T, o) const Def* normalize_ ## o(const Def*, const Def*, Debug);
    THORIN_W_OP(CODE)
#undef CODE

template<MOp > const Def* normalize_MOp (const Def*, const Def*, Debug);
template<IOp > const Def* normalize_IOp (const Def*, const Def*, Debug);
template<ROp > const Def* normalize_ROp (const Def*, const Def*, Debug);
template<ICmp> const Def* normalize_ICmp(const Def*, const Def*, Debug);
template<RCmp> const Def* normalize_RCmp(const Def*, const Def*, Debug);
template<Cast> const Def* normalize_Cast(const Def*, const Def*, Debug);

}
}

#endif
