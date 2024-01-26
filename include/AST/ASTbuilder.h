#pragma once
#include "ASTnode.h"
#include "MxParserVisitor.h"
#include <unordered_map>

namespace dark::AST {

struct ASTbuilder final : public MxParserVisitor {
  public:
    /* Global definitions (Class/Variable/Function). */
    definition_list global;


    ASTbuilder(MxParser::File_InputContext *ctx) { visitFile_Input(ctx); }

  public:   // Override part
    std::any visitFile_Input(MxParser::File_InputContext *) override;
    std::any visitFunction_Definition(MxParser::Function_DefinitionContext *) override;
    std::any visitFunction_Param_List(MxParser::Function_Param_ListContext *) override;
    std::any visitFunction_Argument(MxParser::Function_ArgumentContext *) override;
    std::any visitClass_Definition(MxParser::Class_DefinitionContext *) override;
    std::any visitClass_Ctor_Function(MxParser::Class_Ctor_FunctionContext *) override;
    std::any visitClass_Content(MxParser::Class_ContentContext *) override;
    std::any visitStmt(MxParser::StmtContext *) override;
    std::any visitBlock_Stmt(MxParser::Block_StmtContext *) override;
    std::any visitSimple_Stmt(MxParser::Simple_StmtContext *) override;
    std::any visitBranch_Stmt(MxParser::Branch_StmtContext *) override;
    std::any visitIf_Stmt(MxParser::If_StmtContext *) override;
    std::any visitElse_if_Stmt(MxParser::Else_if_StmtContext *) override;
    std::any visitElse_Stmt(MxParser::Else_StmtContext *) override;
    std::any visitLoop_Stmt(MxParser::Loop_StmtContext *) override;
    std::any visitFor_Stmt(MxParser::For_StmtContext *) override;
    std::any visitWhile_Stmt(MxParser::While_StmtContext *) override;
    std::any visitFlow_Stmt(MxParser::Flow_StmtContext *) override;
    std::any visitVariable_Definition(MxParser::Variable_DefinitionContext *) override;
    std::any visitInit_Stmt(MxParser::Init_StmtContext *) override;
    std::any visitExpr_List(MxParser::Expr_ListContext *) override;
    std::any visitCondition(MxParser::ConditionContext *) override;
    std::any visitSubscript(MxParser::SubscriptContext *) override;
    std::any visitBinary(MxParser::BinaryContext *) override;
    std::any visitFunction(MxParser::FunctionContext *) override;
    std::any visitBracket(MxParser::BracketContext *) override;
    std::any visitMember(MxParser::MemberContext *) override;
    std::any visitConstruct(MxParser::ConstructContext *) override;
    std::any visitUnary(MxParser::UnaryContext *) override;
    std::any visitAtom(MxParser::AtomContext *) override;
    std::any visitLiteral(MxParser::LiteralContext *) override;
    std::any visitTypename(MxParser::TypenameContext *) override;
    std::any visitNew_Type(MxParser::New_TypeContext *) override;
    std::any visitNew_Index(MxParser::New_IndexContext *) override;
    std::any visitLiteral_Constant(MxParser::Literal_ConstantContext *) override;
    std::any visitThis(MxParser::ThisContext *) override;
};


} // namespace dark
