/**
 * No praga once here, because this file should be
 * included only in IRbuilder.h
*/
#include "ASTbase.h"

namespace dark::AST {


struct ExpressionLength : ASTbase {
    ExpressionLength(expression *ctx) { if (ctx) visit(ctx); }
    std::size_t length {};
    void visitBracket(bracket_expr *ctx) { return visit(ctx->expr); }
    void visitSubscript(subscript_expr *ctx) {
        visit(ctx->expr);
        for (auto &i : ctx->subscript) visit(i);
    }
    void visitFunction(function_expr *ctx) {
        visit(ctx->expr);
        for (auto &i : ctx->args) visit(i);
    }
    void visitUnary(unary_expr *ctx) {
        ++length;
        visit(ctx->expr);
    }
    void visitBinary(binary_expr *ctx) {
        ++length;
        visit(ctx->lval);
        visit(ctx->rval);
    }
    void visitTernary(ternary_expr *ctx) {
        length += 3;
        visit(ctx->cond);
        visit(ctx->lval);
        visit(ctx->rval);
    }
    void visitMember(member_expr *ctx) {
        ++length;
        visit(ctx->expr);
    }
    void visitConstruct(construct_expr *ctx) {
        ++length;
        for (auto &i : ctx->subscript) visit(i);
    }
    void visitAtomic(atomic_expr *ctx) { ++length; }
    void visitLiteral(literal_expr *ctx) { ++length; }

    void visitFor(for_stmt *) {}
    void visitWhile(while_stmt *) {}
    void visitFlow(flow_stmt *) {}
    void visitBlock(block_stmt *) {}
    void visitBranch(branch_stmt *) {}
    void visitSimple(simple_stmt *) {}    

    void visitVariableDef(variable_def *) {}
    void visitFunctionDef(function_def *) {}
    void visitClassDef(class_def *) {}
};





} // namespace dark::AST
