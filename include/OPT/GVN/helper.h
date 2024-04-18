#pragma once
#include "IRnode.h"
#include "GVN/match.h"

namespace dark::IR::__gvn {

using enum binary_stmt::binary_op;
using enum type_t;

/**
 * Helper function to create a binary operation match
*/
template <enum binary_stmt::binary_op _Op, typename _P0, typename _P1>
static inline auto m(_P0 &&__p0, _P1 &&__p1) {
    return m_binary <(int)_Op, _P0, _P1> (std::forward<_P0>(__p0), std::forward<_P1>(__p1));
}

/* Match and bind a negative value. */
static inline auto m_negative(number_t &__arg) {
    return m <MUL> (m_value_as(__arg), m_const_is(-1));
}

/**
 * Swap constant to right as much as possible.
 * If none is constant, swap larger index to right.
 * @note Do not apply this to non-commutative operation.
*/
static inline void try_formatize(number_t &__lval, number_t &__rval) {
    if (__lval.is_const()) return std::swap(__lval, __rval);
    if (__rval.is_const()) return void(); // Do nothing.
    if (std::size_t(__lval) > std::size_t(__rval))
        return std::swap(__lval, __rval); // Swap larger hash to right.
}

#define match_return(__arg0, __arg1, __arg2) \
{ if (this->match(__arg0, __arg1)) return __arg2; }


} // namespace dark::IR::__gvn
