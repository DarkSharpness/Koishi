#include "GVN/helper.h"
#include "GVN/algebraic.h"

namespace dark::IR::__gvn {

void algebraicSimplifier::visitADD(number_t __lval, number_t __rval) {
    try_formatize(__lval, __rval);
    /* x + x = x * 2 */
    if (__lval == __rval) return visitMUL(__lval, 2);

    number_t y {};
    int      c {};

    if (const int d = __rval.get_const(); __rval.is_const()) {
        /* c + d = (c + d) */
        if (const int c = __lval.get_const(); __lval.is_const()) return set_result(c + d);

        /* x + 0 = x */
        if (d == 0) return set_result(__lval);

        if (__lval.has_type(BINARY)) {
            /* (x + c) + d = x + (c + d) */
            match_return(__lval, m <ADD> (m_value_as(y), m_const_as(c)), visitADD(y, c + d));

            /* (c - x) + d = (c + d) - x */
            match_return(__lval, m <SUB> (m_const_as(c), m_value_as(y)), visitSUB(c + d, y));
        }

        /* Fail to simplify now. */
        return set_result(ADD, __lval, __rval);
    }

    if (const auto x = __lval; __rval.has_type(BINARY)) {
        /* x + (-y) = x - y */
        match_return(__rval, m_negative(y), visitSUB(x, y));

        /* x + (y - x) = y */
        match_return(__rval, m <SUB> (m_value_as(y), m_value_is(x)), set_result(y));

        /* x + (x * c) = x * (c + 1) */
        match_return(__rval, m <MUL> (m_value_is(x), m_const_as(c)), visitMUL(x, c + 1));

        /* y * c + y * d = y * (c + d) */
        if (int d {}; __lval.has_type(BINARY)
         && match(__lval, m <MUL> (m_value_as(y), m_const_as(c)))
         && match(__rval, m <MUL> (m_value_is(y), m_const_as(d))))
            return visitMUL(y, c + d);
    }

    if (const auto x = __rval; __lval.has_type(BINARY)) {
        /* (-y) + x = x - y */
        match_return(__lval, m_negative(y), visitSUB(x, y));

        /* (y - x) + x = y */
        match_return(__lval, m <SUB> (m_value_as(y), m_value_is(x)), set_result(y));

        /* (x * c) + x = x * (c + 1) */
        match_return(__lval, m <MUL> (m_value_is(x), m_const_as(c)), visitMUL(x, c + 1));
    }

    return set_result(ADD, __lval, __rval); 
}

void algebraicSimplifier::visitSUB(number_t __lval, number_t __rval) {
    /* x - c = x + (-c). */
    if (__rval.is_const())  return visitADD(__lval, -__rval.get_const());
    /* 0 - x = -x */
    if (__lval.is_const(0)) return visitNEG(__rval);
    /* x - x = 0 */
    if (__lval == __rval)   return set_result(0);

    number_t y {};
    int      c {};

    if (const auto x = __lval; __rval.has_type(BINARY)) {
        /* x - (-y) = x + y */
        match_return(__rval, m_negative(y), visitADD(x, y));

        /* x - (x - y) = y */
        match_return(__rval, m <SUB> (m_value_is(x), m_value_as(y)), set_result(y));

        /* x - (x + y) = (-y) */
        match_return(__rval, m <ADD> (m_value_is(x), m_value_as(y)), visitNEG(y));
        match_return(__rval, m <ADD> (m_value_as(y), m_value_is(x)), visitNEG(y));

        /* x - (x * c) = x * (1 - c) */
        match_return(__rval, m <MUL> (m_value_is(x), m_const_as(c)), visitMUL(x, 1 - c));

        if (int d = x.get_const(); x.is_const()) {
            /* d - (y + c) = (d - c) - y */
            match_return(__rval, m <ADD> (m_value_as(y), m_const_as(c)), visitSUB(d - c, y));

            /* d - (c - y) = y + (d - c) */
            match_return(__rval, m <SUB> (m_const_as(c), m_value_as(y)), visitADD(y, d - c));
        }

        /* (y * c) - (y * d) = y * (c - d) */
        if (int d {}; __lval.has_type(BINARY)
         && match(__lval, m <MUL> (m_value_as(y), m_const_as(c)))
         && match(__rval, m <MUL> (m_value_is(y), m_const_as(d)))) return visitMUL(y, c - d);
    }

    if (const auto x = __rval; __lval.has_type(BINARY)) {
        /* (-y) - y = y * -2 */
        match_return(__lval, m_negative(y), visitMUL(y, -2));

        /* (x - y) - x = (-y) */
        match_return(__lval, m <SUB> (m_value_is(x), m_value_as(y)), visitNEG(y));

        /* (x + y) - x = y */
        match_return(__lval, m <ADD> (m_value_as(y), m_value_is(x)), set_result(y));
        match_return(__lval, m <ADD> (m_value_is(x), m_value_as(y)), set_result(y));

        /* (x * c) - x = x * (c - 1) */
        match_return(__lval, m <MUL> (m_value_is(x), m_const_as(c)), visitMUL(x, c - 1));
    }

    return set_result(SUB, __lval, __rval);
}

void algebraicSimplifier::visitNEG(number_t __rval) {
    /* 0 - (-c) = c */
    if (__rval.is_const()) return set_result(-__rval.get_const());

    if (!__rval.has_type(BINARY)) return set_result(MUL, __rval, -1);

    number_t x {};
    int      c {};

    /* 0 - (x + c) = -c - x */
    match_return(__rval, m <ADD> (m_value_as(x), m_const_as(c)), visitSUB(-c, x));

    /* 0 - (x - y) = y - x */
    number_t y {};
    match_return(__rval, m <SUB> (m_value_as(x), m_value_as(y)), visitSUB(y, x));

    /* 0 - (x * c) = x * -c */
    match_return(__rval, m <MUL> (m_value_as(x), m_const_as(c)), visitMUL(x, -c));

    /* 0 - (c << x) = (-c) << x */
    match_return(__rval, m <SHL> (m_const_as(c), m_value_as(x)), set_result(SHL, -c, x));

    return set_result(MUL, __rval, -1);
}


} // namespace dark::IR::__gvn
