#pragma once
#include "utility.h"
namespace dark::IR { struct integer_constant; }
namespace dark::IR::__gvn {

enum class type_t {
    UNKNOWN = 0,// unknown
    BINARY,     // binary
    COMPARE,    // compare
    GETADDR,    // addr + offset
    CONSTANT,   // constant
};

/**
 * The number assigned to an expression.
 * 
 * If the number is constant, then the data field
 * within is the constant value.
 * 
 * Otherwise, the number is an abstract label index.
 * We can use the index to find the corresponding expression.
 */
struct number_t {
  private:
    int    data = {};
    type_t type = {};
  public:
    explicit number_t() = default;
    explicit operator std::size_t() const { return data; }

    number_t(int __data, type_t __tp = type_t::CONSTANT) : data(__data), type(__tp) {}

    bool is_const()             const { return type == type_t::CONSTANT; }
    bool is_const(int __val)    const { return is_const() && data == __val; }
    bool has_type(type_t __tp)   const { return type == __tp; }

    /* This function is safe to call even in wrong case. */
    int get_const() const { return data; }
    /* This function is safe to call even in wrong case. */
    int get_index() const { return data; }

    friend bool operator == (number_t, number_t) = default;
};

/**
 * An expression type with a left and right value.
 * It can be used as a key in a hash table.
 */
struct expression {
  protected:
    type_t type;    // Type of the expression.
    int operand;    // Operand if some.
    number_t lval;  // Left value.
    number_t rval;  // Right value.
  public:
    explicit expression() = default;
    expression(type_t __tp, int __op, number_t __l, number_t __r)
        : type(__tp), operand(__op), lval(__l), rval(__r) {}

    friend bool operator == (expression, expression) = default;

    number_t get_l() const { return lval; }
    number_t get_r() const { return rval; }
    int get_op() const { return operand; }

    /* An almost unique hash implementation. */
    std::size_t hash() const {
        return
            static_cast <std::size_t> (type)    << 0
        |   static_cast <std::size_t> (operand) << 4
        |   static_cast <std::size_t> (lval)    << 8
        |   static_cast <std::size_t> (rval)    << 32;
    }
};

/* Simple hash wrapper. */
struct custom_hash {
    std::size_t operator()(expression __e) const { return __e.hash(); }
};


} // namespace dark::IR::__gvn
