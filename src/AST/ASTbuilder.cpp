#include "ASTnode.h"
#include "ASTbuilder.h"
#include <ranges>

namespace dark::AST {

inline std::string Mx_string_parse(std::string &__src) {
    std::string __dst;
    if(__src.front() != '\"' || __src.back() != '\"')
        throw error("Invalid string literal.");
    __src.pop_back();
    for(size_t i = 1 ; i < __src.length() ; ++i) {
        if(__src[i] == '\\') {
            switch(__src[++i]) {
                case 'n':  __dst.push_back('\n'); break;
                case '\"': __dst.push_back('\"'); break;
                case '\\': __dst.push_back('\\'); break;
                default: throw error("Invalid escape sequence.");
            }
        } else __dst.push_back(__src[i]);
    } return __dst;
}

std::any ASTbuilder::visitFile_Input(MxParser::File_InputContext *ctx) {
    for (auto __p : ctx->children) {
        if (__p->getText() == "<EOF>") break;

        /* Require the node to be definition. */
        global.push_back(get_node <definition> (__p));
    }
    return {};
}

std::any ASTbuilder::visitFunction_Definition(MxParser::Function_DefinitionContext *ctx) {
    auto *__func = pool.allocate <function_def> ();

    /* Set the return type and function name.  */
    if (auto *__p = ctx->function_Argument() ; true) {
        static_cast <argument &> (*__func) = argument {
            .type = get_value <typeinfo> (__p->typename_()),
            .name = __p->Identifier()->getText(),
        };
    }

    /* Build up function argument list. */
    if (auto * __args = ctx->function_Param_List()) {
        for (auto __p : __args->function_Argument())
            __func->args.push_back(argument {
                .type = get_value <typeinfo> (__p->typename_()),
                .name = __p->Identifier()->getText(),
        });
    }

    __func->body = get_node <statement> (ctx->block_Stmt());
    return set_node(__func);
}

std::any ASTbuilder::visitClass_Definition(MxParser::Class_DefinitionContext *ctx) {
    auto *__class = pool.allocate <class_def> ();
    __class->name = ctx->Identifier()->getText();
    for (auto __p : ctx->class_Content())
        __class->member.push_back(get_node <definition> (__p));
    return set_node(__class);
}

std::any ASTbuilder::visitClass_Content(MxParser::Class_ContentContext *ctx) {
    return visitChildren(ctx);
}

std::any ASTbuilder::visitClass_Ctor_Function(MxParser::Class_Ctor_FunctionContext *ctx) {
    auto *__ctor = pool.allocate <function_def> ();
    /* Constructor function is unqiue , with its name = "" */
    __ctor->type = typeinfo {
        .base       = get_class(ctx->Identifier()->getText()),
        .dimensions = 0,
        .assignable = false,
    };
    __ctor->body = get_node <statement> (ctx->block_Stmt());
    return set_node(__ctor);
}

std::any ASTbuilder::visitStmt(MxParser::StmtContext *ctx) {
    return visitChildren(ctx);
}

std::any ASTbuilder::visitBlock_Stmt(MxParser::Block_StmtContext *ctx) {
    auto *__block = pool.allocate <block_stmt> ();
    for (auto __p : ctx->stmt())
        __block->stmt.push_back(get_node <statement> (__p));
    return set_node(__block);
}

std::any ASTbuilder::visitSimple_Stmt(MxParser::Simple_StmtContext *ctx) {
    auto *__simple = pool.allocate <simple_stmt> ();
    if (auto *__list = ctx->expr_List())
        __simple->expr = get_value <expression_list> (__list);
    return set_node(__simple);
}

std::any ASTbuilder::visitBranch_Stmt(MxParser::Branch_StmtContext *ctx) {
    using branch_t = branch_stmt::branch_t;
    auto *__branch = pool.allocate <branch_stmt> ();

    /* Always contains if statment. */
    if (auto *__p = ctx->if_Stmt(); true) {
        __branch->branches.push_back(branch_t {
            .cond = get_node <expression> (__p->expression()),
            .body = get_node <statement>  (__p->stmt()),
        });
    }

    /* Some else-if statments. */
    for (auto __p : ctx->else_if_Stmt()) {
        __branch->branches.push_back(branch_t {
            .cond = get_node <expression> (__p->expression()),
            .body = get_node <statement>  (__p->stmt()),
        });
    }

    /* May contain else statment. */
    if (auto *__p = ctx->else_Stmt())
        __branch->else_body = get_node <statement> (__p->stmt());

    return set_node(__branch);
}


std::any ASTbuilder::visitLoop_Stmt(MxParser::Loop_StmtContext *ctx) {
    return visitChildren(ctx);
}

std::any ASTbuilder::visitFor_Stmt(MxParser::For_StmtContext *ctx) {
    auto * __loop = pool.allocate <for_stmt> ();
    /* Initializer. */
    if (auto *__p = ctx->simple_Stmt()) {
        __loop->init = get_node <statement> (__p);
    } else if (auto *__q = ctx->variable_Definition()) {
        __loop->init = get_node <statement> (__q);
    } else {
        runtime_assert(false, "This should never happen");
        __builtin_unreachable();
    }

    /* Condition. */
    if (auto *__p = ctx->condition)
        __loop->cond = get_node <expression> (__p);

    /* Increment. */
    if (auto *__p = ctx->step)
        __loop->step = get_node <expression> (__p);

    __loop->body = get_node <statement> (ctx->stmt());
    return set_node(__loop);
}

std::any ASTbuilder::visitWhile_Stmt(MxParser::While_StmtContext *ctx) {
    auto *__loop = pool.allocate <while_stmt> ();
    __loop->cond = get_node <expression> (ctx->expression());
    __loop->body = get_node <statement> (ctx->stmt());
    return set_node(__loop);
}

std::any ASTbuilder::visitFlow_Stmt(MxParser::Flow_StmtContext *ctx) {
    auto *__flow = pool.allocate <flow_stmt> ();
    if (ctx->Break()) {
        __flow->sort = flow_stmt::BREAK;
    } else if (ctx->Continue()) {
        __flow->sort = flow_stmt::CONTINUE;
    } else {
        runtime_assert(ctx->Return(), "This should never happen");
        __flow->sort = flow_stmt::RETURN;
        if (auto *__p = ctx->expression())
            __flow->expr = get_node <expression> (__p);
    }
    return set_node(__flow);
}

std::any ASTbuilder::visitVariable_Definition(MxParser::Variable_DefinitionContext *ctx) {
    auto *__var = pool.allocate <variable_def> ();
    __var->type = get_value <typeinfo> (ctx->typename_());
    __var->type.assignable = true;  // Variable is assignable by default.
    for (auto __p : ctx->init_Stmt()) {
        auto *__init = __p->expression();
        __var->vars.push_back(variable_pair {
            .name = __p->Identifier()->getText(),
            .expr = __init ? get_node <expression> (__init) : nullptr,
        });
    }
    return set_node(__var);
}

std::any ASTbuilder::visitExpr_List(MxParser::Expr_ListContext *ctx) {
    expression_list __list;
    for (auto __p : ctx->expression())
        __list.push_back(get_node <expression> (__p));
    return __list;
}

std::any ASTbuilder::visitCondition(MxParser::ConditionContext *ctx) {
    auto *__cond = pool.allocate <ternary_expr> ();
    __cond->cond = get_node <expression> (ctx->v);
    __cond->lval = get_node <expression> (ctx->l);
    __cond->rval = get_node <expression> (ctx->r);
    return set_node(__cond);
}

std::any ASTbuilder::visitSubscript(MxParser::SubscriptContext *ctx) {
    auto *__subsript = pool.allocate <subscript_expr> ();
    __subsript->expr = get_node <expression> (ctx->l);
    for (auto __p : ctx->expression() | std::views::drop(1))
        __subsript->subscript.push_back(get_node <expression> (__p));
    return set_node(__subsript);
}

std::any ASTbuilder::visitBinary(MxParser::BinaryContext *ctx) {
    auto *__binary = pool.allocate <binary_expr> ();
    __binary->op   = ctx->op->getText();
    __binary->lval = get_node <expression> (ctx->l);
    __binary->rval = get_node <expression> (ctx->r);
    return set_node(__binary);
}

std::any ASTbuilder::visitFunction(MxParser::FunctionContext *ctx) {
    auto *__function = pool.allocate <function_expr> ();
    __function->func = get_node <expression> (ctx->l);
    if (auto *__p = ctx->expr_List())
        __function->args = get_value <expression_list> (__p);
    return set_node(__function);
}

std::any ASTbuilder::visitBracket(MxParser::BracketContext *ctx) {
    return visit(ctx->expression());
}

std::any ASTbuilder::visitMember(MxParser::MemberContext *ctx) {
    auto *__member = pool.allocate <member_expr> ();
    __member->expr = get_node <expression> (ctx->l);
    __member->name = ctx->Identifier()->getText();
    return set_node(__member);
}

std::any ASTbuilder::visitConstruct(MxParser::ConstructContext *__tmp) {
    auto *ctx = __tmp->new_Type(); // Real ctx pointer.
    auto *__construct = pool.allocate <construct_expr> ();

    /* Build up subscript , and part of type information. */
    if (auto *__idx = ctx->new_Index()) {
        runtime_assert(!ctx->Paren_Left_(), "Parenthesis is not allowed in array initialization.");
        runtime_assert(__idx->bad.empty(), "Bad array index format.");
        for (auto __p : __idx->good)
            __construct->subscript.push_back(get_node <expression> (__p));
        /* Set the dimensions. */
        __construct->type.dimensions = static_cast <int> (__idx->Brack_Left_().size());
    }

    /* Set the class pointer. */
    std::string __str = ctx->Identifier() ?
        ctx->Identifier()->getText() : ctx->BasicTypes()->getText();
    __construct->type.base = get_class(std::move(__str));

    return set_node(__construct);
}

std::any ASTbuilder::visitUnary(MxParser::UnaryContext *ctx) {
    auto *__unary = pool.allocate <unary_expr> ();
    if (ctx->l) {   // Suffix "++" will be denoted as "+++"
        __unary->op   = ctx->op->getText();
        __unary->expr = get_node <expression> (ctx->l);
        __unary->op.str[2] = __unary->op.str[0]; 
    } else {
        __unary->op   = ctx->op->getText();
        __unary->expr = get_node <expression> (ctx->r);
    }
    return set_node(__unary);
}

std::any ASTbuilder::visitAtom(MxParser::AtomContext *ctx) {
    auto *__atom = pool.allocate <atomic_expr> ();
    __atom->name = ctx->getText();
    return set_node(__atom);
}

std::any ASTbuilder::visitLiteral(MxParser::LiteralContext *ctx) {
    auto *__literal = pool.allocate <literal_expr> ();
    auto *__ptr     = ctx->literal_Constant();
    __literal->name = ctx->getText();

    if      (__ptr->Number())   __literal->sort = literal_expr::NUMBER;
    else if (__ptr->Null())     __literal->sort = literal_expr::_NULL_;
    else if (__ptr->Cstring())  __literal->sort = literal_expr::STRING,
                                __literal->name = Mx_string_parse(__literal->name);
    else                        __literal->sort = literal_expr::_BOOL_;
    return set_node(__literal);
}

std::any ASTbuilder::visitTypename(MxParser::TypenameContext *ctx) {
    std::string __str = ctx->Identifier() ?
        ctx->Identifier()->getText() : ctx->BasicTypes()->getText();
    return typeinfo {
        .base       = get_class(std::move(__str)),
        .dimensions = static_cast <int> (ctx->Brack_Left_().size()),
        .assignable = false,
    };
}

std::any ASTbuilder::visitThis(MxParser::ThisContext *) {
    auto *__atom = pool.allocate <atomic_expr> ();
    __atom->name = "this";
    return set_node(__atom);
}



/* These parts are manually inlined. */
std::any ASTbuilder::visitFunction_Param_List(MxParser::Function_Param_ListContext *ctx) {
    runtime_assert(false, "Function parameter list should not be visited.");
    __builtin_unreachable();
}
std::any ASTbuilder::visitFunction_Argument(MxParser::Function_ArgumentContext *) {
    runtime_assert(false, "Function argument should not be visited.");
    __builtin_unreachable();
}
std::any ASTbuilder::visitIf_Stmt(MxParser::If_StmtContext *) {
    runtime_assert(false, "If statement should not be visited.");
    __builtin_unreachable();
}
std::any ASTbuilder::visitElse_if_Stmt(MxParser::Else_if_StmtContext *) {
    runtime_assert(false, "Else if statement should not be visited.");
    __builtin_unreachable();
}
std::any ASTbuilder::visitElse_Stmt(MxParser::Else_StmtContext *) {
    runtime_assert(false, "Else statement should not be visited.");
    __builtin_unreachable();
}
std::any ASTbuilder::visitInit_Stmt(MxParser::Init_StmtContext *) {
    runtime_assert(false, "Init statement should not be visited.");
    __builtin_unreachable();
}
std::any ASTbuilder::visitNew_Type(MxParser::New_TypeContext *) {
    runtime_assert(false, "New type should not be visited.");
    __builtin_unreachable();
}
std::any ASTbuilder::visitNew_Index(MxParser::New_IndexContext *) {
    runtime_assert(false, "New index should not be visited.");
    __builtin_unreachable();
}
std::any ASTbuilder::visitLiteral_Constant(MxParser::Literal_ConstantContext *) {
    runtime_assert(false, "Literal constant should not be visited.");
    __builtin_unreachable();
}


} // namespace dark
