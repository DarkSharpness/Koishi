#pragma once
#include "ASTbase.h"
#include "IRbase.h"
#include <deque>

namespace dark::IR {


struct IRbuilder : AST::ASTbase {
  private:
    function *top           {}; // Current function working.
    block    *top_block     {}; // Current block working.
    AST::scope * const global_scope;

    definition *visit_value {}; // The value we are visiting.
    bool        is_lvalue   {}; // Whether the value is a lvalue
    bool        is_ready    {}; // Whether the value is ready to be used.

    struct branch_setting {
        block *branch[2];   // [0]: false branch. [1]: true branch.
        void negate() { std::swap(branch[0], branch[1]); }
    };

    std::deque <branch_setting> branch_stack;

    definition *get_value();
    non_literal *get_address();

    void set_value(definition *);
    void set_address(non_literal *);
    void set_invalid();
    void add_block(block *);
    void end_block(flow_statement *);

    branch_stmt *get_branch();

  public:

    std::unordered_map <std::string, class_type *>      class_map;
    std::unordered_map <AST::identifier *,function *>   function_map;
    std::unordered_map <AST::identifier *,variable *>   variable_map;

    std::deque  <custom_type>       global_classes;     // Class types
    std::deque  <function>          global_functions;   // Global functions
    std::deque  <global_variable>   global_variables;   // Global variables

    function *main_function {};

    /**
     * Variable which is used to represent a implicit member acccess.
     * In member function, the first parameter is always the pointer to the class.
     * In body part, "this" pointer is always implicit.
     * Operation "this->member" is equivalent to "member".
     * So, when we are dealing with atomic expression, we need to know
     * whether it is a local variable or a member variable.
    */
    inline static local_variable member_variable {};

  public:
    IRbuilder(AST::ASTbuilder *, AST::scope *);

    std::string IRtree() const;

    void visitBracket(AST::bracket_expr *) override;
    void visitSubscript(AST::subscript_expr *) override;
    void visitFunction(AST::function_expr *) override;
    void visitUnary(AST::unary_expr *) override;
    void visitBinary(AST::binary_expr *) override;
    void visitTernary(AST::ternary_expr *) override;
    void visitMember(AST::member_expr *) override;
    void visitConstruct(AST::construct_expr *) override;
    void visitAtomic(AST::atomic_expr *) override;
    void visitLiteral(AST::literal_expr *) override;

    void visitFor(AST::for_stmt *) override;
    void visitWhile(AST::while_stmt *) override;
    void visitFlow(AST::flow_stmt *) override;
    void visitBlock(AST::block_stmt *) override;
    void visitBranch(AST::branch_stmt *) override;
    void visitSimple(AST::simple_stmt *) override;

    void visitVariableDef(AST::variable_def *) override;
    void visitFunctionDef(AST::function_def *) override;
    void visitClassDef(AST::class_def *) override;

    void register_class(AST::class_def *);
    void create_builtin(AST::ASTbuilder *);
    void create_class(AST::class_def *);
    void create_function(AST::function_def *,bool);
    void visitGlobalVariable(AST::variable *, AST::literal_expr *);

    typeinfo transform_type(AST::typeinfo);

    void visitShortCircuit(AST::binary_expr *, bool);
    void visitShortCircuitPhi(AST::binary_expr *, bool);
    void visitAssign(AST::binary_expr *);
    void visitCompare(AST::binary_expr *, int);
    void visitNewArray(typeinfo, std::vector <definition *>);
};


} // namespace dark::IR
