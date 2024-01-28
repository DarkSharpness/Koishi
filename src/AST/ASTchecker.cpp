#include "ASTnode.h"
#include "ASTscope.h"
#include "ASTscanner.h"
#include "ASTbuilder.h"
#include "ASTchecker.h"

namespace dark::AST {

ASTchecker::ASTchecker(ASTbuilder *__builder) {
    class_scanner(__builder, &alloc);
    function_scanner(__builder, &alloc);

}


void ASTchecker::visitSubscript(subscript_expr *) {}
void ASTchecker::visitFunction(function_expr *) {}
void ASTchecker::visitUnary(unary_expr *) {}
void ASTchecker::visitBinary(binary_expr *) {}
void ASTchecker::visitTernary(ternary_expr *) {}
void ASTchecker::visitMember(member_expr *) {}
void ASTchecker::visitConstruct(construct_expr *) {}
void ASTchecker::visitAtomic(atomic_expr *) {}
void ASTchecker::visitLiteral(literal_expr *) {}

void ASTchecker::visitFor(for_stmt *) {}
void ASTchecker::visitWhile(while_stmt *) {}
void ASTchecker::visitReturn(return_stmt *) {}
void ASTchecker::visitFlow(flow_stmt *) {}
void ASTchecker::visitBlock(block_stmt *) {}
void ASTchecker::visitBranch(branch_stmt *) {}
void ASTchecker::visitSimple(simple_stmt *) {}    

void ASTchecker::visitVariableDef(variable_def *) {}
void ASTchecker::visitFunctionDef(function_def *) {}
void ASTchecker::visitClassDef(class_def *) {}


} // namespace dark::AST
