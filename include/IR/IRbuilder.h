#pragma once
#include "ASTbase.h"
#include "IRbase.h"
#include <deque>

namespace dark::IR {


struct IRbuilder : AST::ASTbase {
  private:
    std::unordered_map <std::string, class_type *>          class_map;
    std::unordered_map <AST::identifier *,IR::function *> function_map;
    std::unordered_map <AST::identifier *,IR::variable *> variable_map;

    std::deque   <function>         global_functions;   // Global functions
    std::deque  <global_variable>   global_variables;   // Global variables

  public:
    IRbuilder(AST::ASTbuilder *);

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
};


} // namespace dark::IR
