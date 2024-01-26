#include "ASTbuilder.h"

namespace dark::AST {

std::any ASTbuilder::visitFile_Input(MxParser::File_InputContext *ctx) {

}

std::any ASTbuilder::visitFunction_Definition(MxParser::Function_DefinitionContext *) {

}

std::any ASTbuilder::visitFunction_Param_List(MxParser::Function_Param_ListContext *) {}
std::any ASTbuilder::visitFunction_Argument(MxParser::Function_ArgumentContext *) {}
std::any ASTbuilder::visitClass_Definition(MxParser::Class_DefinitionContext *) {}
std::any ASTbuilder::visitClass_Ctor_Function(MxParser::Class_Ctor_FunctionContext *) {}
std::any ASTbuilder::visitClass_Content(MxParser::Class_ContentContext *) {}
std::any ASTbuilder::visitStmt(MxParser::StmtContext *) {}
std::any ASTbuilder::visitBlock_Stmt(MxParser::Block_StmtContext *) {}
std::any ASTbuilder::visitSimple_Stmt(MxParser::Simple_StmtContext *) {}
std::any ASTbuilder::visitBranch_Stmt(MxParser::Branch_StmtContext *) {}
std::any ASTbuilder::visitIf_Stmt(MxParser::If_StmtContext *) {}
std::any ASTbuilder::visitElse_if_Stmt(MxParser::Else_if_StmtContext *) {}
std::any ASTbuilder::visitElse_Stmt(MxParser::Else_StmtContext *) {}
std::any ASTbuilder::visitLoop_Stmt(MxParser::Loop_StmtContext *) {}
std::any ASTbuilder::visitFor_Stmt(MxParser::For_StmtContext *) {}
std::any ASTbuilder::visitWhile_Stmt(MxParser::While_StmtContext *) {}
std::any ASTbuilder::visitFlow_Stmt(MxParser::Flow_StmtContext *) {}
std::any ASTbuilder::visitVariable_Definition(MxParser::Variable_DefinitionContext *) {}
std::any ASTbuilder::visitInit_Stmt(MxParser::Init_StmtContext *) {}
std::any ASTbuilder::visitExpr_List(MxParser::Expr_ListContext *) {}
std::any ASTbuilder::visitCondition(MxParser::ConditionContext *) {}
std::any ASTbuilder::visitSubscript(MxParser::SubscriptContext *) {}
std::any ASTbuilder::visitBinary(MxParser::BinaryContext *) {}
std::any ASTbuilder::visitFunction(MxParser::FunctionContext *) {}
std::any ASTbuilder::visitBracket(MxParser::BracketContext *) {}
std::any ASTbuilder::visitMember(MxParser::MemberContext *) {}
std::any ASTbuilder::visitConstruct(MxParser::ConstructContext *) {}
std::any ASTbuilder::visitUnary(MxParser::UnaryContext *) {}
std::any ASTbuilder::visitAtom(MxParser::AtomContext *) {}
std::any ASTbuilder::visitLiteral(MxParser::LiteralContext *) {}
std::any ASTbuilder::visitTypename(MxParser::TypenameContext *) {}
std::any ASTbuilder::visitNew_Type(MxParser::New_TypeContext *) {}
std::any ASTbuilder::visitNew_Index(MxParser::New_IndexContext *) {}
std::any ASTbuilder::visitLiteral_Constant(MxParser::Literal_ConstantContext *) {}
std::any ASTbuilder::visitThis(MxParser::ThisContext *) {}


} // namespace dark
