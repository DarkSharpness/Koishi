#include "ASTnode.h"
#include "ASTscope.h"
#include "ASTscanner.h"
#include "ASTbuilder.h"
#include "ASTchecker.h"

namespace dark::AST {

static bool is_convertible(typeinfo __src, typeinfo __dst) {
    if (__src == __dst) return true;
    if (__src.data() == "null") {
        /* OK. Nullptr can convert to array types. */
        if (__dst.dimensions > 0) return true;
        /* Nullptr can convert to any class types. */
        return __dst.base->name != "void" && __dst.base->name != "int"
            && __dst.base->name != "bool" && __dst.base->name != "string";
    } else return false;
}

ASTchecker::ASTchecker(ASTbuilder *__builder)
    : class_map(__builder->classes), pool(__builder->pool) {
    /* Initializing. */
    class_scanner(__builder, &alloc);
    global = function_scanner(__builder, &alloc).global;
    void_class = &class_map["void"];

    /* Working...... */
    create_init();
    for (auto *__def : __builder->global) { top = global; visit(__def); }
    check_main();
    visit_init();
    /* If need init, just push it back. */
    if (global_init) {
        __builder->global.push_back(global_init);
        global->insert(global_init);

        auto *__main = safe_cast <function *> (global->find("main"));
        auto *__block = safe_cast <block_stmt *> (__main->body);

        auto *__atom = __builder->pool.allocate <atomic_expr> ();
        __atom->name = "_Global_Init";
        __atom->real = global_init;

        auto *__call = __builder->pool.allocate <function_expr> ();
        __call->func = __atom;
        __call->type = get_type("void");

        auto *__stmt = __builder->pool.allocate <simple_stmt> ();
        __stmt->expr = { __call };

        __block->stmt.insert(__block->stmt.begin(), __stmt);
    }
}

void ASTchecker::visitBracket(bracket_expr *ctx) {
    ctx->field = top;
    visit(ctx->expr);
    ctx->type = ctx->expr->type;
}

void ASTchecker::visitSubscript(subscript_expr *ctx) {
    ctx->field = top;
    for (auto __p : ctx->subscript) {
        visit(__p);
        runtime_assert(__p->type == get_type("int"), "Subscript must be integer");
    }
    visit(ctx->expr);

    /* Subscripts may be assignable. */
    auto __type = ctx->expr->type;
    __type.dimensions -= ctx->subscript.size();
    __type.assignable  = true;
    runtime_assert(__type.dimensions >= 0, "Too many subscripts");
    ctx->type = __type;
}

void ASTchecker::visitFunction(function_expr *ctx) {
    ctx->field = top;
    visit(ctx->func);
    auto &__type = ctx->func->type;     // Should be function type.
    runtime_assert(__type.is_function(), "Not a function");
    auto *__func = __type.base->func;   // Real function pointer.
    runtime_assert(__func->args.size() == ctx->args.size(),
        "Wrong number of arguments");
    auto  __args = __func->args.begin();
    for (auto __p : ctx->args) {
        visit(__p);
        runtime_assert(
            is_convertible(__p->type, __args->type),
            "Wrong argument type"
        ); ++__args;
    }
    /* Return values are no assignable. */
    ctx->type = __func->type;
}

void ASTchecker::visitUnary(unary_expr *ctx) {
    ctx->field = top;
    visit(ctx->expr);
    const auto &__type = ctx->expr->type;
    if (ctx->op.str[1]) { // "++" or "--" case.
        runtime_assert(__type == get_type("int") && __type.assignable,
            "Invalid self-inc/dec");
        // Only prefix ++/-- is assignable.
        ctx->type = __type;
        ctx->type.assignable = (ctx->op.str[2] == '\0'); 
    } else {
        switch (ctx->op.str[0]) {
            case '+':   case '-':   case '~':
                runtime_assert(__type == get_type("int"),
                    "Invalid unary operator"); break;
            case '!':
                runtime_assert(__type == get_type("bool"),
                    "Invalid unary operator"); break;
            default:
                runtime_assert(false, "Unexpected unary operator");
                __builtin_unreachable();
        }
        // Other unary operators are not assignable.
        ctx->type = __type;
        ctx->type.assignable = false;
    }
}

void ASTchecker::visitBinary(binary_expr *ctx) {
    ctx->field = top;
    visit(ctx->lval);
    visit(ctx->rval);
    const auto &__ltype = ctx->lval->type;
    const auto &__rtype = ctx->rval->type;

    runtime_assert(!__ltype.is_function() && !__rtype.is_function(),
        "Function cannot be binary operand");
    runtime_assert(!is_void(__ltype) && !is_void(__rtype),
        "Void cannot be binary operand");

    /* Assignment. */
    if (ctx->op == "=") {
        runtime_assert(__ltype.assignable,
            "LHS not assignable in assignment");
        runtime_assert(is_convertible(__rtype, __ltype),
            "RHS not convertible to LHS in assignment");
        ctx->type = __ltype;
        return;
    }

    /* Non-assignment case. */
    if (__ltype == __rtype && !__ltype.dimensions) {
        auto __name = __ltype.data();
        if (__name == "int") {
            ctx->type = get_type(type_checker::check_int(ctx));
        } else if (__name == "bool") {
            ctx->type = get_type(type_checker::check_bool(ctx));
        } else if (__name == "string") {
            ctx->type = get_type(type_checker::check_string(ctx));
        } else {
            ctx->type = get_type(type_checker::check_other(ctx));
        }
    } else {
        runtime_assert(
            is_convertible(__rtype, __ltype) || is_convertible(__ltype, __rtype),
            "Incompatible types in binary expression"
        );
        ctx->type = get_type(type_checker::check_other(ctx));
    }
}

void ASTchecker::visitTernary(ternary_expr *ctx) {
    ctx->field = top;
    visit(ctx->cond);
    runtime_assert(ctx->cond->type == get_type("bool"),
        "Condition must be bool");

    visit(ctx->lval);
    visit(ctx->rval);
    const auto &__ltype = ctx->lval->type;
    const auto &__rtype = ctx->rval->type;

    if (is_convertible(__rtype, __ltype)) {
        ctx->type = __ltype;
    } else if (is_convertible(__ltype, __rtype)) {
        ctx->type = __rtype;
    } else {
        runtime_assert(false, "Incompatible types in ternary expression");
    }

    /* We do not support left-value ternery expression! */
    ctx->type.assignable = false;
}

void ASTchecker::visitMember(member_expr *ctx) {
    ctx->field = top;
    visit(ctx->expr);
    const auto &__type = ctx->expr->type;
    runtime_assert(!__type.is_function(), "Function cannot be member operand");
    if (__type.dimensions > 0) {
        /* Actually, only .size() is valid. */
        auto __member = class_map["_Array"].field->find(ctx->name);
        runtime_assert(__member, "Member not found for \"_Array\" type");
        ctx->type = get_type(__member);
    } else {
        auto __member = __type.base->field->find(ctx->name);
        runtime_assert(__member, "Member not found for class type");
        ctx->type = get_type(__member);
    }
}

void ASTchecker::visitConstruct(construct_expr *ctx) {
    ctx->field = top;
    runtime_assert(!is_void(ctx->type), "Cannot construct void object!");
    for (auto __p : ctx->subscript) {
        visit(__p);
        runtime_assert(__p->type == get_type("int"), "Subscript must be integer");
    }
    ctx->type.assignable = false;
}

void ASTchecker::visitAtomic(atomic_expr *ctx) {
    ctx->field = top;
    auto __atomic = top->find(ctx->name);
    runtime_assert(__atomic, "No such identifier \"",ctx->name,"\"");
    ctx->type = get_type(__atomic);
    ctx->real = __atomic;
}

void ASTchecker::visitLiteral(literal_expr *ctx) {
    ctx->field = top;
    switch (ctx->sort) {
        case literal_expr::NUMBER: ctx->type = get_type("int");     return;
        case literal_expr::STRING: ctx->type = get_type("string");  return;
        case literal_expr::_BOOL_: ctx->type = get_type("bool");    return;
        case literal_expr::_NULL_: ctx->type = get_type("null");    return;
        default: runtime_assert(false, "This should not happen");   break;
    }
    __builtin_unreachable();
}

void ASTchecker::visitFor(for_stmt *ctx) {
    ctx->field = top = alloc.allocate(top);
    if (ctx->init) visit(ctx->init);
    if (ctx->cond) {
        visit(ctx->cond);
        runtime_assert(ctx->cond->type == get_type("bool"),
            "Condition must be bool");
    }
    if (ctx->step) visit(ctx->step);

    loop_stack.push_back(ctx);
    visit(ctx->body);
    loop_stack.pop_back();
    top = top->prev;
}

void ASTchecker::visitWhile(while_stmt *ctx) {
    ctx->field = top = alloc.allocate(top);
    visit(ctx->cond);
    runtime_assert(ctx->cond->type == get_type("bool"),
        "Condition must be bool");

    loop_stack.push_back(ctx);
    visit(ctx->body);
    loop_stack.pop_back();
    top = top->prev;
}

void ASTchecker::visitFlow(flow_stmt *ctx) {
    ctx->field = top;
    if (ctx->sort == flow_stmt::RETURN) {
        runtime_assert(top_function, "Return outside function");
        if (ctx->expr) {
            visit(ctx->expr);
            runtime_assert(
                is_convertible(ctx->expr->type, top_function->type),
                "Return type mismatch"
            );
        } else {
            runtime_assert(top_function->type == get_type("void"),
                "Return type mismatch");
        }
        ctx->func = top_function;
    } else { // Continue or Break.
        runtime_assert(!loop_stack.empty(), "Break outside loop");
        ctx->loop = loop_stack.back();
    }
}


void ASTchecker::visitBlock(block_stmt *ctx) {
    ctx->field = top = alloc.allocate(top);
    for (auto __p : ctx->stmt) visit(__p);
    top = top->prev;
}

void ASTchecker::visitBranch(branch_stmt *ctx) {
    ctx->field = top;
    for (auto [__cond, __body] : ctx->branches) {
        visit(__cond);
        runtime_assert(__cond->type == get_type("bool"),
            "Condition must be bool");
        top = alloc.allocate(top);
        visit(__body);
        top = top->prev;
    }

    if (ctx->else_body) {
        top = alloc.allocate(top);
        visit(ctx->else_body);
        top = top->prev;
    }
}

void ASTchecker::visitSimple(simple_stmt *ctx) {
    ctx->field = top;
    for (auto __p : ctx->expr) visit(__p);
}    

void ASTchecker::visitVariableDef(variable_def *ctx) {
    ctx->field = top;
    runtime_assert(!is_void(ctx->type), "Cannot declare void variable");
    for (auto &[__name,__init] : ctx->vars) {
        if (__init) {
            visit(__init);
            runtime_assert(
                is_convertible(__init->type, ctx->type),
                "Initializer type mismatch"
            );
        }

        auto *__var = create_variable(ctx->type, __name);
        runtime_assert(top->insert(__var), "Duplicate variable name");

        if (top_function) {
            top_function->locals.push_back(__var);
        } else if (__init && !dynamic_cast <literal_expr *> (__init)) {
            /* Such global variables can not be construct directly. */
            auto *__stmt = pool.allocate <simple_stmt> ();
            auto *__expr = pool.allocate <binary_expr> ();
            auto *__atom = pool.allocate <atomic_expr> ();
            __atom->name = __name;
            __atom->real = __var;
            __expr->op   = "=";
            __expr->lval = __atom;
            __expr->rval = __init;
            __stmt->expr.push_back(__expr);
            global_body->stmt.push_back(__stmt);
            __init = nullptr; // Move the initializer to global init.
        }
    }
}

void ASTchecker::visitFunctionDef(function_def *ctx) {
    top_function = ctx;
    top = ctx->field; // Function scope has been created in function_scanner.

    for (auto &[__type, __name] : ctx->args) {
        auto *__var = create_variable(__type, __name);
        runtime_assert(top->insert(__var), "Duplicate argument name");
    }

    visit(ctx->body);
    top_function = nullptr;
    top = top->prev;
}

void ASTchecker::visitClassDef(class_def *ctx) {
    top = ctx->field;
    /**
     * Only check member functions
     * Member variables have been checked in function_scanner.
    */
    for (auto __p : ctx->member)
        if (dynamic_cast <function *> (__p)) visit(__p);

    top = top->prev;
}


void ASTchecker::create_init() {
    global_init = pool.allocate <function_def> ();
    global_init->field  = global;
    global_init->type   = get_type("void");
    global_init->name   = "_Global_Init";
    global_init->unique_name = "._Global_Init";
    global_body         = pool.allocate <block_stmt> ();
    global_init->body   = global_body;
}

void ASTchecker::visit_init() {
    top = global;
    /* If there is no global variable to init, then we do not need global init. */
    auto *__blk = safe_cast <block_stmt *> (global_init->body);
    if (__blk->stmt.empty()) global_init = nullptr;
    else visit(global_init);
}


void ASTchecker::check_main() {
    auto *__func = dynamic_cast <function *> (global->find("main"));
    runtime_assert(__func, "main function not found");
    runtime_assert(__func->args.size() == 0, "main function must have no arguments");
    runtime_assert(__func->type == get_type("int"), "main function must return int");
}

typeinfo ASTchecker::get_type(identifier *__id) {
    if (dynamic_cast <variable *> (__id)) return __id->type;
    /**
     * Note that if __id is a function,
     * then __id->type is the return type of the function.
     * We need to wrap it as a class type.
     */
    auto *__func = safe_cast <function *> (__id);
    auto [__iter, __] = function_map.try_emplace(__func, __func);
    return typeinfo { &__iter->second, 0 , 0 };
}

variable *ASTchecker::create_variable(typeinfo __type, std::string __name) {
    auto *__var = pool.allocate <variable> ();
    __var->type = __type;
    __var->name = __name;
    // Global variable case.
    if (!top_function) __var->unique_name = '@' + __name;
    else { // Local variable case.
        std::size_t  __cnt = variable_count[top_function]++;
        __var->unique_name = '%' + __var->name + '-' + std::to_string(__cnt);
    }
    return __var;
}


} // namespace dark::AST
