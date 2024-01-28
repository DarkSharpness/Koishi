#pragma once
#include "utility.h"
#include <unordered_set>

/* ASTbase and node forward declaration. */
namespace dark::AST {

struct ASTbase;
struct node;
struct scope;
struct class_type;

struct subscript_expr;
struct function_expr;
struct unary_expr;
struct binary_expr;
struct ternary_expr;
struct member_expr;
struct construct_expr;
struct atomic_expr;
struct literal_expr;

struct for_stmt;
struct while_stmt;
struct return_stmt;
struct flow_stmt;
struct block_stmt;
struct branch_stmt;
struct simple_stmt;

struct variable_def;
struct function_def;
struct class_def;

struct node {
    scope *field {}; // pointer to scope
    virtual void print() const = 0;
    virtual void accept(ASTbase *) = 0;
    virtual ~node() = default;
};
/* Visitor base class. */
struct ASTbase {
    void visit(node *__n) { __n->accept(this); }
    virtual ~ASTbase() = default;

    virtual void visitSubscript(subscript_expr *) = 0;
    virtual void visitFunction(function_expr *) = 0;
    virtual void visitUnary(unary_expr *) = 0;
    virtual void visitBinary(binary_expr *) = 0;
    virtual void visitTernary(ternary_expr *) = 0;
    virtual void visitMember(member_expr *) = 0;
    virtual void visitConstruct(construct_expr *) = 0;
    virtual void visitAtomic(atomic_expr *) = 0;
    virtual void visitLiteral(literal_expr *) = 0;

    virtual void visitFor(for_stmt *) = 0;
    virtual void visitWhile(while_stmt *) = 0;
    virtual void visitReturn(return_stmt *) = 0;
    virtual void visitFlow(flow_stmt *) = 0;
    virtual void visitBlock(block_stmt *) = 0;
    virtual void visitBranch(branch_stmt *) = 0;
    virtual void visitSimple(simple_stmt *) = 0;    

    virtual void visitVariableDef(variable_def *) = 0;
    virtual void visitFunctionDef(function_def *) = 0;
    virtual void visitClassDef(class_def *) = 0;
};

struct ASTbuilder;
struct ASTchecker;


} // namespace dark::AST


/* Some basic types. */
namespace dark::AST {

/* Type information. */
struct typeinfo {
    class_type *  base  {}; // Type of expression.
    int     dimensions  {}; // Dimensions of array.
    bool    assignable  {}; // Whether left value.

    /* Return the name of the typeinfo. */
    std::string data() const noexcept;
};

/* Function argument type. */
struct argument {
    typeinfo    type;   // Type of argument.
    std::string name;   // Name of argument.
};

/* Variable/function/class definition. */
struct definition : virtual node {};
/* Expression type. */
struct expression : virtual node { typeinfo type; };
/* Statement type. */
struct statement  : virtual node {};


/* Information of a class type/function type. */
struct class_type {
    const std::string name;   // Name of a function/class
    union {
        void *  impl{}; // Pointer of implementation.
        scope * field;  // Member field of a class.
        function_def *  func;   // Function to call.
    };
    class_type(std::string __str) noexcept : name(std::move(__str)) {}
};

/* Hash function for class */
struct class_hash {
    std::size_t operator()(const class_type &__t) const noexcept {
        return std::hash<std::string>{}(__t.name);
    }
};

/* Equal function for class */
struct class_equal {
    bool operator()(const class_type &__lhs, const class_type &__rhs) const noexcept {
        return __lhs.name == __rhs.name;
    }
};


/* Operand string type. */
struct operand_t {
    char str[8] {};

    operand_t() = default;

    void operator = (std::string  &&  __str) noexcept { return operator = (__str); }
    void operator = (std::string_view __str) noexcept {
        runtime_assert(__str.size() < 8, "operand string too long");
        for (size_t i = 0; i < __str.size(); ++i) str[i] = __str[i];
    }

    template <size_t _N> requires (_N < 8)
    friend bool operator ==
        (const operand_t &__lhs, const char (&__rhs)[_N])
    noexcept {
        for (size_t i = 0; i < _N; ++i)
            if (__lhs.str[i] != __rhs[i])
                return false;
        return true;
    }
};

/* Pair of variable definition. */
struct variable_pair {
    std::string name;       // Name of variable.
    expression *expr {};    // Initial value.
};


/* An identifier can be a function/variable */
struct identifier : argument {
    std::string unique_name;
    virtual ~identifier() = default;
};

/* We use function_def to represent a function identifier. */
using function = function_def;
/* A simple variable as identifier. */
struct variable : node, identifier {
    void print() const override {
        runtime_assert(false, "variable::print() should not be called");
        __builtin_unreachable();
    }
    void accept(ASTbase *) override {
        runtime_assert(false, "variable::accept() should not be called");
        __builtin_unreachable();
    }
};

using definition_list = std::vector <definition *>;
using expression_list = std::vector <expression *>;
using   argument_list = std::vector <argument>;
using   variable_list = std::vector <variable_pair>;
struct class_list : std::unordered_set <class_type, class_hash, class_equal> {
    template <typename ..._Args>
    class_type &operator [] (_Args &&...__args) {
        return const_cast <class_type &> (*this->emplace(
            std::forward <_Args> (__args)...
        ).first);
    }
};


} // namespace dark::AST
