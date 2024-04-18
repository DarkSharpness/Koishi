#pragma once
#include "value.h"
#include <span>

namespace dark::IR::__gvn {

struct matcher {
  protected:
    template <typename _Pattern_t>
    bool match(number_t __n, _Pattern_t &&__p) const { return __p.match(__n); }
};

/* Set the binded value. */
struct m_value_as {
  private:
    number_t * const value;
  public:
    m_value_as(number_t &__v) : value(&__v) {}
    bool match(number_t __n) const { *value = __n; return true; }
};

/* Match whether it is a constant and set the binded value. */
struct m_const_as {
  private:
    int * const value;
  public:
    m_const_as(int &__v) : value(&__v) {}
    bool match(number_t __n) const {
        if (!__n.is_const())        return false;
        *value = __n.get_const();   return true;
    }
};

/* Match whether it is a given value. */
struct m_value_is {
  private:
    number_t const value;
  public:
    m_value_is(number_t __v) : value(__v) {}
    bool match(number_t __n) const { return __n == value; }
};

/* Match whether it is a given constant. */
struct m_const_is {
  private:
    int const value;
  public:
    m_const_is(int __v) : value(__v) {}
    bool match(number_t __n) const { return __n.is_const(value); }
};

/* Match the component of a binary expression. */
template <int _Op, typename _Pattern_0, typename _Pattern_1>
struct m_binary {
  private:
    _Pattern_0 lhs;
    _Pattern_1 rhs;
  public:
    m_binary(_Pattern_0 &&__lhs, _Pattern_1 &&__rhs)
        : lhs(std::move(__lhs)), rhs(std::move(__rhs)) {}

    bool match(number_t __n) const {
        auto &__e = __n.get_expression();
        if (__e.get_op() != _Op) return false;
        return lhs.match(__e.get_l()) && rhs.match(__e.get_r());
    }
};


} // namespace dark::IR::__gvn
