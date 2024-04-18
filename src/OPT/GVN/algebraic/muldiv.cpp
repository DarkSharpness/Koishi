#include "GVN/helper.h"
#include "GVN/algebraic.h"

namespace dark::IR::__gvn {

/**
 * Match whether it is a constant and times of another
 * constant and set the binded value.
*/
template <typename _Func>
struct m_const_if {
  private:
    int * const value;
    _Func func;
  public:
    m_const_if(int &__v, _Func &&__f) : value(&__v), func(std::move(__f)) {}
    bool match(number_t __n) const {
        return __n.is_const() && func(*value = __n.get_const());
    }
};

auto f_equal(int base) { return [base](int __n) -> bool { return __n == base; };    }
auto f_times(int base) { return [base](int __n) -> bool { return __n % base == 0; }; }
auto f_shift(int base) { return [base](int __n) -> bool { return (1 << __n) == base; }; }

/* Return whether a number is a negative binary expression. */
bool algebraicSimplifier::is_negative(number_t __n, number_t &__x) const {
    return __n.has_type(BINARY) && match(__n, m_negative(__x));
}

void algebraicSimplifier::visitMUL(number_t __lval, number_t __rval) {
    try_formatize(__lval, __rval);
    number_t x {};
    int      c {};

    if (int d = __rval.get_const(); __rval.is_const()) {
        /* x * 0 = 0 */
        if (d == 0) return set_result(0);
        /* x * 1 = x */
        if (d == 1) return set_result(__lval);
        /* x * -1 = -x */
        if (d == -1) return visitNEG(__lval);
        /* c * d = (c * d) */
        if (int c = __lval.get_const(); __lval.is_const())
            return set_result(c * d);

        /* Can not simplify. */
        if (!__lval.has_type(BINARY)) return set_result(MUL, __lval, __rval);

        /* (x * c) * d = x * (c * d) */
        match_return(__lval, m <MUL> (m_value_as(x), m_const_as(c)), visitMUL(x, c * d));

        /* (x << c) * d = x * (c << d) */
        match_return(__lval, m <SHL> (m_value_as(x), m_const_as(c)), visitMUL(x, c << d));

        /* (c << x) * d = (c * d) << x */
        match_return(__lval, m <SHL> (m_const_as(c), m_value_as(x)), visitSHL(c * d, x));

        /* (x >> c) * d = x & -d (if 1 << c == d) */
        match_return(__lval, m <SHR> (m_value_as(x), m_const_if(c, f_shift(d))), visitAND(x, -d));

        return set_result(MUL, __lval, __rval);
    }

    number_t y {};
    /* (-x) * (-y) = x * y */
    if (is_negative(__lval, x) && is_negative(__rval, y))
        return visitMUL(x, y);
    else
        return set_result(MUL, __lval, __rval);
}

/* Magic number. */
static inline constexpr int UB_default = /* 1919810 */ 0;

void algebraicSimplifier::visitDIV(number_t __lval, number_t __rval) {
    /* x / x = 1 */
    if (__lval == __rval) return set_result(1);

    number_t x {};
    int      c {};

    if (int d = __rval.get_const(); __rval.is_const()) {
        /* x / 1 = x */
        if (d == 1)  return set_result(__lval);
        /* x / 0 = undefined */
        if (d == 0)  return set_result(UB_default);
        /* x / -1 = -x */
        if (d == -1) return visitNEG(__lval);
        /* c / d = (c / d) */
        if (int c = __lval.get_const(); __lval.is_const())
            return set_result(c / d);

        /* Can not simplify. */
        if (!__lval.has_type(BINARY)) return set_result(DIV, __lval, __rval);

        /* (x * c) / d = x * (c / d) (when c % d == 0)*/
        match_return(__lval, m <MUL> (m_value_as(x), m_const_if(c, f_times(d))), visitMUL(x, c / d));

        /* (x / c) / d = x / (c * d) */
        match_return(__lval, m <DIV> (m_value_as(x), m_const_as(c)), visitDIV(x, c * d));

        /* (-x) / c = x / (-c) */
        match_return(__lval, m_negative(x), visitDIV(x, -c));

        return set_result(DIV, __lval, __rval);
    }

    if (const auto y = __rval; __rval.has_type(BINARY)) {
        /* (x * y) / y = x */
        match_return(__lval, m <MUL> (m_value_as(x), m_value_is(y)), set_result(x));
        match_return(__lval, m <MUL> (m_value_is(y), m_value_as(x)), set_result(x));
    }

    number_t y {};

    auto __lneg = is_negative(__lval, x);
    auto __rneg = is_negative(__rval, y);

    /* (0 - x) / x = -1 */
    if (__lneg && x == y) return set_result(-1);

    /* x / (0 - x) = -1 */
    if (__rneg && x == y) return set_result(-1);

    /* c / (0 - y) = -c / y */
    if (__rneg && __lval.is_const()) return visitDIV(-__lval.get_const(), y);

    /* (0 - x) / (0 - y) = x / y */
    if (__lneg && __rneg) return visitDIV(x, y);
}

void algebraicSimplifier::visitMOD(number_t __lval, number_t __rval) {
    /* x % x = 0 */
    if (__lval == __rval) return set_result(0);

    number_t x {};
    int      c {};

    if (int d = __rval.get_const(); __rval.is_const()) {
        /* x % 0 = undefined */
        if (d == 0) return set_result(UB_default);
        /* x % 1 = 0 */
        if (d == 1 || d == -1) return set_result(0);
        /* c % d = (c % d) */
        if (int c = __lval.get_const(); __lval.is_const())
            return set_result(c % d);

        /* Can not simplify. */
        if (!__lval.has_type(BINARY)) return set_result(MOD, __lval, __rval);

        /* (x * c) % d = 0 */
        match_return(__lval, m <MUL> (m_value_as(x), m_const_if(c, f_times(d))), set_result(0));

        return set_result(MOD, __lval, __rval);
    }

    /* x % (0 - y) = x % y */
    if (number_t y {}; is_negative(__rval, y)) return visitMOD(__lval, y);

    if (const auto y = __rval; __rval.has_type(BINARY)) {
        /* (x * y) % y = 0 */
        match_return(__lval, m <MUL> (m_value_as(x), m_value_is(y)), set_result(0));
        match_return(__lval, m <MUL> (m_value_is(y), m_value_as(x)), set_result(0));
    }

    return set_result(MOD, __lval, __rval);
}

} // namespace dark::IR::__gvn
