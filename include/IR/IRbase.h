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
    const literal *init {}; // Initial value of the global variable.
    bool    is_constant {}; // If true, the global variable's value is a constant.
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
struct IRbuilder;

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

using  statement = node;
struct flow_statement : statement {};
struct memory_statement : statement {};

struct IRbase {
    void visit(node * __n) { return __n->accept(this); }

    virtual void visitCompare(compare_stmt *) = 0;
    virtual void visitBinary(binary_stmt *) = 0;
    virtual void visitJump(jump_stmt *) = 0;
    virtual void visitBranch(branch_stmt *) = 0;
    virtual void visitCall(call_stmt *) = 0;
    virtual void visitLoad(load_stmt *) = 0;
    virtual void visitStore(store_stmt *) = 0;
    virtual void visitReturn(return_stmt *) = 0;
    virtual void visitGet(get_stmt *) = 0;
    virtual void visitPhi(phi_stmt *) = 0;
    virtual void visitUnreachable(unreachable_stmt *) = 0;

    virtual ~IRbase() = default;
};

/**
 * @brief A block consists of:
 * 1. Phi functions
 * 2. Statements
 * 3. Control flow statement
 * 
 * Hidden impl is intended to provide future
 * support for loop or other info.
 */
struct block final : hidden_impl {
    std::vector <phi_stmt *> phi;   // All phi functions
    flow_statement *flow {};        // Control flow statement
    std::string     name;           // Label name

    std::vector <statement*>    data;   // All normal statements
    std::vector  <block *>      prev;   // Predecessor blocks
    fixed_vector <block *, 2>   next;   // Successor blocks

    void push_phi(phi_stmt *);          // Push back a phi function
    void push_back(statement *);        // Push back a statement
    void print(std::ostream &) const;   // Print the block data
    bool is_unreachable() const;        // Is this block unreachable?
};

struct function final : hidden_impl {
  private:
    std::size_t loop_count {};  // Count of for loops
    std::size_t cond_count {};  // Count of branches
    std::unordered_map <std::string, std::size_t> temp_count; // Count of temporaries

  public:
    typeinfo    type;       // Return type
    std::string name;       // Function name
    std::vector  <block  *>  data;   // All blocks
    std::vector <argument *> args;   // Arguments
    std::vector <local_variable *> locals; // Local variables

    bool is_builtin {};
    bool has_input  {};
    bool has_output {};

    temporary *create_temporary(typeinfo, const std::string &);

    void push_back(block *);
    // void push_back(statement *);     // To avoid misusage.
    void print(std::ostream &) const;   // Print the function data
    bool is_unreachable() const;        // Is this function unreachable?
};

/* A global memory pool for IR. */
struct IRpool {
  private:
    IRpool() = delete;
    inline static central_allocator <node>        pool1 {};
    inline static central_allocator <non_literal> pool2 {};
    using _Function_Ptr = function *;

  public:
    inline static pointer_constant *__null__    {};
    inline static integer_constant *__zero__    {};
    inline static integer_constant *__pos1__    {};
    inline static integer_constant *__neg1__    {};
    inline static boolean_constant *__true__    {};
    inline static boolean_constant *__false__   {};

    static const _Function_Ptr builtin_function;

    /* Pool from string to global_variable initialized by string. */
    inline static std::unordered_map <std::string, global_variable> str_pool {};

    /* Initialize the pool. */
    static void init_pool();
    /* Allocate one node. */
    template <typename _Tp, typename... _Args>
    requires 
        (!std::same_as <unreachable_stmt, _Tp>
    &&  std::constructible_from <_Tp, _Args...>
    &&  std::derived_from <_Tp, statement>)
    static _Tp *allocate(_Args &&...__args) {
        return pool1.allocate <_Tp> (std::forward <_Args> (__args)...);
    }

    /* Allocate one non_literal. */
    template <std::derived_from <non_literal> _Tp>
    static _Tp *allocate() { return pool2.allocate <_Tp> (); }

    /* Allocate unreachable. */
    template <std::same_as <unreachable_stmt> _Tp>
    static unreachable_stmt *allocate();

    /* Allocate one block. */
    template <std::same_as <block> _Tp>
    static block *allocate();

    static integer_constant *create_integer(int);
    static boolean_constant *create_boolean(bool);
    static pointer_constant *create_pointer(const global_variable *);
    static global_variable  *create_string(const std::string &);
    static undefined *create_undefined(typeinfo, int = 0);
};


} // namespace dark::IR
