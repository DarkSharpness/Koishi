#pragma once
#include "utility.h"

/* Definitions used. */
namespace dark::IR {

struct integer_constant;
struct definition;    

} // namespace dark::IR

namespace dark::IR::__gvn {

enum class type_t {
    UNKNOWN = 0,// unknown
    BINARY,     // binary
    COMPARE,    // compare
    GETADDR,    // addr + offset
    CONSTANT,   // constant
};

struct expression;

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
    using _Ptr_t = const expression *;
    using _Ref_t = const expression &;
    _Ptr_t  expr = {};
    type_t  type = {};
    int     data = {};
  public:
    explicit number_t() = default;

    number_t(int __const_value)
        : expr(nullptr), type(type_t::CONSTANT), data(__const_value) {}

    explicit number_t(_Ptr_t __e);

    /* For hash use only. Do not abuse it. */
    explicit operator std::size_t() const
    { return std::hash <const void *> {} (expr) + static_cast <std::size_t> (type); }

    bool is_const()             const { return expr == nullptr; }
    bool is_const(int __val)    const { return is_const() && get_const() == __val; }
    bool has_type(type_t __tp)  const { return type == __tp; }

    /* This function is safe to call even in wrong case. */
    int get_const() const { return data; }
    /* This function is not safe to call in wrong case. */
    _Ref_t get_expression() const { return *expr; }

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
    explicit expression(int __unknown_id)
        : type(type_t::UNKNOWN), operand(__unknown_id), lval(), rval() {}

    explicit expression(type_t __tp, int __op, number_t __l, number_t __r)
        : type(__tp), operand(__op), lval(__l), rval(__r) {}

    number_t get_l()    const { return lval; }
    number_t get_r()    const { return rval; }
    int      get_op()   const { return operand; }
    type_t get_type()   const { return type; }

    /* An almost unique hash implementation. */
    std::size_t hash() const {
        return
            static_cast <std::size_t> (type)    << 0
        |   static_cast <std::size_t> (operand) << 4
        |   static_cast <std::size_t> (lval)    << 8
        |   static_cast <std::size_t> (rval)    << 32;
    }

    friend bool operator == (expression, expression) = default;
};

inline number_t::number_t(_Ptr_t __e) : expr(__e), type(__e->get_type()), data() {}

/* Simple hash wrapper. */
struct custom_hash {
    std::size_t operator()(expression __e) const { return __e.hash(); }
};

/* A set of equal elements. */
struct equal_set {
    definition *best = nullptr;

    /* Insert a definition pointer to the equal-set. */
    void insert(definition *);
    /**
     * Remove a definition pointer from the equal-set. 
     * @note This operation will move the pointer to
     * a cache pool, which just means that this pointer
     * has gone out of its live range.
    */
    void remove(definition *);
    /* Return the best element in the equal-set. */
    definition *top();
};


} // namespace dark::IR::__gvn
