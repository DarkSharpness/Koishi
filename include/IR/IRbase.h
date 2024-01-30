#pragma once
#include "IRtype.h"
#include <string>
#include <vector>
#include <unordered_map>

/* Some basic definitions */
namespace dark::IR {

/* A definition can be variable / literal / temporary */
struct definition {
    virtual std::string data()          const = 0;
    virtual typeinfo get_value_type()   const = 0;
    virtual ~definition() = default;
    typeinfo get_point_type() const { return --get_value_type(); }
};

/* A special type whose value is undefined.  */
struct undefined final: definition {
    typeinfo type;
    explicit undefined(typeinfo __type) : type(__type) {}
    typeinfo get_value_type()   const override;
    std::string        data()   const override;
    ~undefined() override = default;
};

/* Literal constants. */
struct literal;
/* Non literal value. (Variable / Temporary) */
struct non_literal : definition {
    typeinfo    type; /* Type wrapper. */
    std::string name; /* Unique name.  */
    typeinfo get_value_type()   const override final { return type; }
    std::string        data()   const override final { return name; }
};

/* Temporary values. */
struct temporary final : non_literal {};
/* Variables are pointers to value. */
struct variable : non_literal {};
/* Function parameters. */
struct argument         final  : variable {};
struct local_variable   final  : variable {};
struct global_variable  final  : variable {
    /* If constantly initialized, this is the variable's value. */
    const literal *const_init {};
    bool is_const() const noexcept { return const_init != nullptr; }
};

/* Literal constants. */
struct literal : definition {
    using ssize_t = std::make_signed_t <size_t>;
    /* Type that is used to be print out in global variable declaration. */
    virtual std::string IRtype() const = 0;
    /* Return the inner integer representative. */
    virtual ssize_t to_integer() const = 0;
};

/* String literal. */
struct string_constant final : literal {
    std::string     context;
    cstring_type    stype;
    explicit string_constant(std::string __ctx) :
        context(std::move(__ctx)), stype(context.size()) {}

    std::string IRtype() const override;
    ssize_t to_integer() const override;
    std::string  data()  const override;
    typeinfo get_value_type() const override;
    std::string     ASMdata() const;
};

/* Integer literal. */
struct integer_constant final : literal {
    const int value;
    explicit integer_constant(int __val) : value(__val) {}
    std::string IRtype() const override;
    ssize_t to_integer() const override;
    std::string   data() const override;
    typeinfo get_value_type() const override;
};

/* Boolean literal */
struct boolean_constant final : literal {
    const bool value;
    explicit boolean_constant(bool __val) : value(__val) {}
    std::string IRtype() const override;
    ssize_t to_integer() const override;
    std::string   data() const override;
    typeinfo get_value_type() const override;
};

/**
 * Pointer constant.
 * 
 * If (.var != nullptr) :
 * global_variable @str -> string_constant.
 * pointer_constant     -> global_variable @str.
 * 
 * Else, it represents just "nullptr"
*/
struct pointer_constant final : literal {
    const global_variable *const var;
    explicit pointer_constant(const global_variable * __var)
        : var(__var) {}
    std::string IRtype() const override;
    ssize_t to_integer() const override;
    std::string   data() const override;
    typeinfo get_value_type() const override;
};


} /* namespace dark::IR */


/* IR visitor part. */
namespace dark::IR {

struct IRbase;

struct node {
    using _Def_List = std::vector <definition *>;
    virtual void accept(IRbase * __v) = 0;
    /* Return the string form IR. */
    virtual std::string data() const = 0;
    /* Return the temporary this statment defines. */
    virtual temporary *get_def() const = 0;
    /* Return all the usages of the node. */
    virtual _Def_List  get_use() const = 0;
    /* To avoid memory leak. */ 
    virtual ~node() = default;
};

/* Declaration. */
struct block_stmt;
struct function;
struct initialization;
struct compare_stmt;
struct binary_stmt;
struct jump_stmt;
struct branch_stmt;
struct call_stmt;
struct load_stmt;
struct store_stmt;
struct return_stmt;
struct alloca_stmt;
struct get_stmt;
struct phi_stmt;
struct unreachable_stmt;

struct IRbase {
    void visit(node * __n) { return __n->accept(this); }

    virtual void visitBlock(block_stmt *) = 0;
    virtual void visitFunction(function *) = 0;
    virtual void visitInit(initialization *) = 0;

    virtual void visitCompare(compare_stmt *) = 0;
    virtual void visitBinary(binary_stmt *) = 0;
    virtual void visitJump(jump_stmt *) = 0;
    virtual void visitBranch(branch_stmt *) = 0;
    virtual void visitCall(call_stmt *) = 0;
    virtual void visitLoad(load_stmt *) = 0;
    virtual void visitStore(store_stmt *) = 0;
    virtual void visitReturn(return_stmt *) = 0;
    virtual void visitAlloca(alloca_stmt *) = 0;
    virtual void visitGet(get_stmt *) = 0;
    virtual void visitPhi(phi_stmt *) = 0;
    virtual void visitUnreachable(unreachable_stmt *) = 0;

    virtual ~IRbase() = default;
};

/* A global memory pool for IR. */
struct IRpool {
  private:
    IRpool() = delete;
    inline static central_allocator <node>        pool1 {};
    inline static central_allocator <non_literal> pool2 {};
  public:
    inline static pointer_constant *__null__    {};
    inline static integer_constant *__zero__    {};
    inline static integer_constant *__pos1__    {};
    inline static integer_constant *__neg1__    {};
    inline static boolean_constant *__true__    {};
    inline static boolean_constant *__false__   {};

    /* Pool from string to global_variable initialized by string. */
    inline static std::unordered_map <std::string, global_variable> str_pool {};

    /* Initialize the pool. */
    static void init_pool();
    /* Allocate one node. */
    template <typename _Tp>
    static _Tp *allocate_node() { return pool1.allocate <_Tp> (); }
    /* Allocate one non_literal. */
    template <typename _Tp>
    static _Tp *allocate_def()  { return pool2.allocate <_Tp> (); }

    static integer_constant *create_integer(int);
    static boolean_constant *create_boolean(bool);
    static pointer_constant *create_pointer(const global_variable *);
    static global_variable  *create_string(const std::string &);
    static undefined *create_undefined(typeinfo, int = 0);
};

}
