#pragma once
#include "ASTbase.h"
#include "ASTscope.h"
#include <unordered_map>

namespace dark::AST {

struct ASTchecker final : ASTbase {
  private:
    using _Map_t        = class_list;
    using _Alloc_Node   = central_allocator <node>;
    using _Alloc_Scope  = central_allocator <scope>;

    scope *global   {};     // global scope
    scope *top      {};     // top scope currently

    _Map_t &class_map;          // class map
    _Alloc_Node    &pool;       // allocator for nodes
    _Alloc_Scope    alloc;      // allocator for scope

    std::vector <loop_type *> loop_stack;   // Loop stack
    function *  top_function {};            // Current function
    function *  global_init;                // Global init function.
    block_stmt *global_body;                // Global init function body
    class_type *void_class;                 // Void class.

    /* Map of functions to their class_type. */
    std::unordered_map <function *,class_type> function_map;
    /* Map of functions to its variable count. */
    std::unordered_map <function *,std::size_t> variable_count;

    void create_init();
    void check_main();
    void visit_init();

    /* Return the typeinfo of an identifier. */
    typeinfo get_type(identifier *);

    /* Return the typeinfo of a class by its name. */
    template <typename _Str>
    requires std::convertible_to <_Str, std::string>
    typeinfo get_type(_Str &&__str, int __n = 0, bool __a = false) {
        return typeinfo {
            .base = &class_map[std::forward <_Str> (__str)],
            .dimensions = __n,
            .assignable = __a,
        };
    }

    /* Return whether a typeinfo is void or void array. */
    bool is_void(typeinfo __type) const { return __type.base == void_class; }

    /* Create a variable under a function scope. */
    variable *create_variable(typeinfo, std::string);

  public:

    ASTchecker(ASTbuilder *);
    void visitBracket(bracket_expr *) override;
    void visitSubscript(subscript_expr *) override;
    void visitFunction(function_expr *) override;
    void visitUnary(unary_expr *) override;
    void visitBinary(binary_expr *) override;
    void visitTernary(ternary_expr *) override;
    void visitMember(member_expr *) override;
    void visitConstruct(construct_expr *) override;
    void visitAtomic(atomic_expr *) override;
    void visitLiteral(literal_expr *) override;

    void visitFor(for_stmt *) override;
    void visitWhile(while_stmt *) override;
    void visitFlow(flow_stmt *) override;
    void visitBlock(block_stmt *) override;
    void visitBranch(branch_stmt *) override;
    void visitSimple(simple_stmt *) override;    

    void visitVariableDef(variable_def *) override;
    void visitFunctionDef(function_def *) override;
    void visitClassDef(class_def *) override;
};

} // namespace dark::AST

