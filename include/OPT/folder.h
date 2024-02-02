/**
 * @file folding.h
 * @brief This file contains the declaration of constant folding.
 * It is an AST level optimization, which will folding the constant
 * expression whenever it is possible.
 * If undefined behavior is detected, it will stop folding.
*/
#pragma once
#include "ASTbase.h"

namespace dark::AST {

/**
 * @brief An AST-level constant folder.
 * It will not only fold constant expression,
 * but also translate Mx_string_raw to real ASCII string.
 * 
 */
struct constantFolder final : ASTbase {
  private:
    bool update = false;
    expression *current;

    /**
     * @brief Visit and tries to replace the expression.
     */
    void visitReplace(expression *&expr) {
        visit(expr);
        if (update) {
            update = false;
            expr = current;
        }
    }

    void setReplace(expression *expr) {
        update = true;
        current = expr;
    }

    void setNotReplace() { update = false; }

  public:

    constantFolder(ASTbuilder *);

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

    void visitShortCircuit(binary_expr *, bool);
    void visitCompare(binary_expr *,int);

    void visitArithDouble(literal_expr *, literal_expr *, char);
    void visitArithSingle(expression   *, literal_expr *, char);
};

} // namespace dark::AST
