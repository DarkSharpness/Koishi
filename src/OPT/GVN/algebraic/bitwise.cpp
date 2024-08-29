#include "GVN/helper.h"
#include "GVN/algebraic.h"

namespace dark::IR::__gvn {

void algebraicSimplifier::visitSHL(number_t __lval, number_t __rval) {
    /* 0 << c = 0 */
    if (__lval.is_const(0)) return set_result(__lval);

    /* Can not simplify */
    if (!__rval.is_const()) return set_result(SHL, __lval, __rval);

    const int d = __rval.get_const();

    /* x << 0 = x */
    if (d == 0) return set_result(__lval);

    /* x << 32 = 0 */
    if (d >= 32) return set_result(0);

    /* c << d = (c << d) */
    if (__lval.is_const()) return set_result(__lval.get_const() << d);

    /* Can not simplify */
    if (!__lval.has_type(BINARY)) return set_result(SHL, __lval, __rval);

    number_t x {};
    int      c {};

    /* (c << x) << d = (c << d) << x */
    match_return(__lval, m <SHL> (m_const_as(c), m_value_as(x)), set_result(c << d));

    /* (x << c) << d = x << (c + d) */
    match_return(__lval, m <SHL> (m_value_as(x), m_const_as(c)), visitSHL(x, c + d));

    /* (x >> d) << d = x & (-1 << d) */
    match_return(__lval, m <SHR> (m_value_as(x), m_const_is(d)), visitAND(x, -1 << d));

    return set_result(SHL, __lval, __rval);
}


void algebraicSimplifier::visitSHR(number_t __lval, number_t __rval) {
    /* 0 >> x = 0 */
    if (__lval.is_const(0)) return set_result(__lval);

    /* Can not simplify */
    if (!__rval.is_const()) return set_result(SHR, __lval, __rval);

    const int d = __rval.get_const();

    /* x >> 0 = x */
    if (d == 0) return set_result(__lval);

    /* c >> d = (c >> d)*/
    if (__lval.is_const()) return set_result(__lval.get_const() >> d);

    /* Can not simplify */
    if (!__lval.has_type(BINARY)) return set_result(SHR, __lval, __rval);

    number_t x {};
    int      c {};

    /* (c >> x) >> d = (c >> d) >> x */
    match_return(__lval, m <SHR> (m_const_as(c), m_value_as(x)), set_result(c >> d));

    /* (x >> c) >> d = x >> (c + d) */
    match_return(__lval, m <SHR> (m_value_as(x), m_const_as(c)), visitSHR(x, c + d));

    return set_result(SHR, __lval, __rval);
}

void algebraicSimplifier::visitAND(number_t __lval, number_t __rval) {
    try_formatize(__lval, __rval);

    /* x & x = x */
    if (__lval == __rval) return set_result(__lval);

    number_t x {};
    int      c {};
    if (const int d = __rval.get_const(); __rval.is_const()) {
        /* x & 0 = 0 */
        if (d == 0)     return set_result(0);
        /* x & -1 = x */
        if (d == -1)    return set_result(__lval);

        /* c & d = (c & d) */
        if (__lval.is_const()) return set_result(__lval.get_const() & d);

        /* (x & c) & d = x & (c & d) */
        if (__lval.has_type(BINARY))
            match_return(__lval, m <AND> (m_value_as(x), m_const_as(c)), visitAND(x, c & d));
    
        return set_result(AND, __lval, __rval);
    }

    number_t y {};
    if (const auto x = __rval; __lval.has_type(BINARY)) {
        /* (x & y) & x = x & y */
        match_return(__lval, m <AND> (m_value_is(x), m_value_as(y)), visitAND(x, y));
        match_return(__lval, m <AND> (m_value_as(y), m_value_is(x)), visitAND(x, y));

        /* (x | y) & x = x */
        match_return(__lval, m <OR> (m_value_is(x), m_value_as(y)), set_result(x));
        match_return(__lval, m <OR> (m_value_as(y), m_value_is(x)), set_result(x));

        /* (x ^ c) & x = x & ~c */
        match_return(__lval, m <XOR> (m_value_is(x), m_const_as(c)), visitAND(x, ~c));
    }

    if (const auto x = __lval; __rval.has_type(BINARY)) {
        /* x & (x & y) = x & y */
        match_return(__rval, m <AND> (m_value_is(x), m_value_as(y)), visitAND(x, y));
        match_return(__rval, m <AND> (m_value_as(y), m_value_is(x)), visitAND(x, y));

        /* x & (x | y) = x */
        match_return(__rval, m <OR> (m_value_is(x), m_value_as(y)), set_result(x));
        match_return(__rval, m <OR> (m_value_as(y), m_value_is(x)), set_result(x));

        /* x & (x ^ c) = x & ~c */
        match_return(__rval, m <XOR> (m_value_is(x), m_const_as(c)), visitAND(x, ~c));
    }

    return set_result(AND, __lval, __rval);
}

void algebraicSimplifier::visitOR_(number_t __lval, number_t __rval) {
    try_formatize(__lval, __rval);

    /* x | x = x */
    if (__lval == __rval) return set_result(__lval);

    number_t x {};
    int      c {};
    if (const int d = __rval.get_const(); __rval.is_const()) {
        /* x | 0 = x */
        if (d == 0)     return set_result(__lval);
        /* x | -1 = -1 */
        if (d == -1)    return set_result(-1);

        /* c | d = (c | d) */
        if (__lval.is_const()) return set_result(__lval.get_const() | d);

        /* (x | c) | d = x | (c | d) */
        if (__lval.has_type(BINARY))
            match_return(__lval, m <OR> (m_value_as(x), m_const_as(c)), visitOR_(x, c | d));

        return set_result(OR, __lval, __rval);
    }

    number_t y {};
    if (const auto x = __rval; __lval.has_type(BINARY)) {
        /* (x | y) | x = x | y */
        match_return(__lval, m <OR> (m_value_is(x), m_value_as(y)), visitOR_(x, y));
        match_return(__lval, m <OR> (m_value_as(y), m_value_is(x)), visitOR_(x, y));

        /* (x & y) | x = x */
        match_return(__lval, m <AND> (m_value_is(x), m_value_as(y)), set_result(x));
        match_return(__lval, m <AND> (m_value_as(y), m_value_is(x)), set_result(x));

        /* (x ^ y) | x = x | y */
        match_return(__lval, m <XOR> (m_value_is(x), m_value_as(y)), visitOR_(x, y));
    }

    if (const auto x = __lval; __rval.has_type(BINARY)) {
        /* x | (x | y) = x | y */
        match_return(__rval, m <OR> (m_value_is(x), m_value_as(y)), visitOR_(x, y));
        match_return(__rval, m <OR> (m_value_as(y), m_value_is(x)), visitOR_(x, y));

        /* x | (x & y) = x */
        match_return(__rval, m <AND> (m_value_is(x), m_value_as(y)), set_result(x));
        match_return(__rval, m <AND> (m_value_as(y), m_value_is(x)), set_result(x));

        /* x | (x ^ y) = x | y */
        match_return(__rval, m <XOR> (m_value_is(x), m_value_as(y)), visitOR_(x, y));
    }

    return set_result(OR, __lval, __rval);
}


void algebraicSimplifier::visitXOR(number_t __lval, number_t __rval) {
    /* First, formatize the expression. */
    try_formatize(__lval, __rval);

    /* x ^ x = 0 */
    if (__lval == __rval) return set_result(0);

    number_t x {};
    int      c {};
    if (const int d = __rval.get_const(); __rval.is_const()) {
        /* x ^ 0 = x */
        if (d == 0)     return set_result(__lval);

        /* c ^ d = (c ^ d) */
        if (__lval.is_const()) return set_result(__lval.get_const() ^ d);

        /* (x ^ c) ^ d = x ^ (c ^ d) */
        if (__lval.has_type(BINARY))
            match_return(__lval, m <XOR> (m_value_as(x), m_const_as(c)), visitXOR(x, c ^ d));

        return set_result(XOR, __lval, __rval);
    }

    number_t y {};
    if (const auto x = __rval; __lval.has_type(BINARY)) {
        /* (x ^ y) ^ x = y */
        match_return(__lval, m <XOR> (m_value_is(x), m_value_as(y)), set_result(y));
        match_return(__lval, m <XOR> (m_value_as(y), m_value_is(x)), set_result(y));

        /* (x & c) ^ x = x & ~c */
        match_return(__lval, m <AND> (m_value_is(x), m_const_as(c)), visitAND(x, ~c));
    }

    if (const auto x = __lval; __rval.has_type(BINARY)) {
        /* x ^ (x ^ y) = y */
        match_return(__rval, m <XOR> (m_value_is(x), m_value_as(y)), set_result(y));
        match_return(__rval, m <XOR> (m_value_as(y), m_value_is(x)), set_result(y));

        /* x ^ (x & c) = x & ~c */
        match_return(__rval, m <AND> (m_value_is(x), m_const_as(c)), visitAND(x, ~c));
    }

    return set_result(XOR, __lval, __rval);
}





} // namespace dark::IR::__gvn
