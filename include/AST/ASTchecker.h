#pragma once
#include "ASTbase.h"
#include "ASTscope.h"
#include <unordered_set>

namespace dark::AST {

struct ASTchecker final : ASTbase {
  private:
    using _Map_t = std::unordered_set <class_type, class_hash, class_equal>;
    using _Alloc = central_allocator <scope>;

    scope *global {};   // global scope
    _Map_t class_map;   // class map
    _Alloc alloc;       // allocator for scope

  public:

    ASTchecker(ASTbuilder *);
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
    void visitReturn(return_stmt *) override;
    void visitFlow(flow_stmt *) override;
    void visitBlock(block_stmt *) override;
    void visitBranch(branch_stmt *) override;
    void visitSimple(simple_stmt *) override;    

    void visitVariableDef(variable_def *) override;
    void visitFunctionDef(function_def *) override;
    void visitClassDef(class_def *) override;
};

} // namespace dark::AST

