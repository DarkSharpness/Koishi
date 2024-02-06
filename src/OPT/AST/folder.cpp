#include "AST/folder.h"
#include "ASTnode.h"
#include "ASTbuilder.h"
#include <compare>

namespace dark::AST {

static bool is_AST_string(typeinfo __tp) { return __tp.base->name == "string" && __tp.dimensions == 0; }

constantFolder::constantFolder(ASTbuilder *ctx) {
    for (auto *__p : ctx->global) visit(__p);
}

void constantFolder::visitBracket(bracket_expr *ctx) {
    visitReplace(ctx->expr);
    return setReplace(ctx->expr);
}

void constantFolder::visitSubscript(subscript_expr *ctx) {
    visitReplace(ctx->expr);
    for (auto &o : ctx->subscript) { visitReplace(o); }
    return setNotReplace();
}

void constantFolder::visitFunction(function_expr *ctx) {
    visitReplace(ctx->expr);
    for (auto &o : ctx->args) { visitReplace(o); }
    return setNotReplace();
}

void constantFolder::visitUnary(unary_expr *ctx) {
    visitReplace(ctx->expr);
    auto *__lit = dynamic_cast <literal_expr *> (ctx->expr);
    if (!__lit) return setNotReplace();
    /* Requires to be an literal. */
    char __op = ctx->op.str[0];
    if (__op == '+') {
        runtime_assert(__lit->sort == __lit->NUMBER, "wtf?");
        return setReplace(__lit);
    }
    if (__op == '-') {
        runtime_assert(__lit->sort == __lit->NUMBER, "wtf?");
        if (__lit->name[0] != '-') __lit->name.insert(0, 1, '-');
        else __lit->name.erase(0, 1);
        return setReplace(__lit);
    }
    if (__op == '~') {
        runtime_assert(__lit->sort == __lit->NUMBER, "wtf?");
        __lit->name = std::to_string(~std::stoi(__lit->name));
        return setReplace(__lit);
    }
    if (__op == '!') {
        runtime_assert(__lit->sort == __lit->_BOOL_, "wtf?");
        __lit->name = __lit->name == "true" ? "false" : "true";
        return setReplace(__lit);
    }
}

void constantFolder::visitBinary(binary_expr *ctx) {
    visitReplace(ctx->lval);
    visitReplace(ctx->rval);
    auto *__lval = dynamic_cast <literal_expr *> (ctx->lval);
    auto *__rval = dynamic_cast <literal_expr *> (ctx->rval);
    /* At least of one of them should be a literal. */
    if (!__lval && !__rval) return setNotReplace();

    if (!ctx->op.str[1]) {
        if (ctx->op == "=") return setNotReplace();
        if (ctx->op == "<") return visitCompare(ctx, 2);
        if (ctx->op == ">") return visitCompare(ctx, 3);
    } else if (!ctx->op.str[2]) {
        if (ctx->op == "&&") return visitShortCircuit(ctx, 0);
        if (ctx->op == "||") return visitShortCircuit(ctx, 1);
        if (ctx->op == "!=") return visitCompare(ctx, 0);
        if (ctx->op == "==") return visitCompare(ctx, 1);
        if (ctx->op == "<=") return visitCompare(ctx, 4);
        if (ctx->op == ">=") return visitCompare(ctx, 5);
    }

    /* Except for string add, all the following contains only integers. */
    if (ctx->op == "+" && is_AST_string(ctx->type)) {
        if (__lval && __lval->name == "") return setReplace(ctx->rval);
        if (__rval && __rval->name == "") return setReplace(ctx->lval);
        if (__lval && __rval) {
            __lval->name += __rval->name;
            return setReplace(__lval);
        } else {
            return setNotReplace();
        }
    }

    char __op = ctx->op.str[0];
    if (__lval && __rval) {
        return visitArithDouble(__lval, __rval, __op);
    } else { /* First, tries to format the expression. */
        if (!__rval) {
            switch (__op) /* Fail to optimize if not commutative. */
                case '-': case '/': case '%': case '<': case '>':
                    return setNotReplace();
            std::swap(__lval, __rval);
            std::swap(ctx->lval, ctx->rval);
        }
        return visitArithSingle(ctx->lval, __rval, ctx->op.str[0]);
    }
}

/* The warning is moved to IR. */
static void warningDivideByZero(expression *__lhs, char __op) {}


/**
 * Where both of the operands are literals.
*/
void constantFolder::visitArithDouble(literal_expr * __lval, literal_expr *__rval, char __op) {
    if (__lval && __rval) {
        int __l = std::stoi(__lval->name);
        int __r = std::stoi(__rval->name);
        switch (__op) {
            case '+': __lval->name = std::to_string(__l + __r); break;
            case '-': __lval->name = std::to_string(__l - __r); break;
            case '*': __lval->name = std::to_string(__l * __r); break;
            case '/':
                if (__r == 0)  // Undefined behavior!
                    return warningDivideByZero(__lval,'/'), setNotReplace();
                __lval->name = std::to_string(__l / __r); break;
            case '%':
                if (__r == 0)  // Undefined behavior!
                    return warningDivideByZero(__lval,'%'), setNotReplace();
                __lval->name = std::to_string(__l % __r); break;
            case '&': __lval->name = std::to_string(__l & __r); break;
            case '|': __lval->name = std::to_string(__l | __r); break;
            case '^': __lval->name = std::to_string(__l ^ __r); break;
            case '<': __lval->name = std::to_string(__l << __r); break;
            case '>': __lval->name = std::to_string(__l >> __r); break;
            default: runtime_assert(false, "wtf?");
        }
        return setReplace(__lval);
    }



}

/**
 * Where exactly one of the operands is a literal.
 * Unary optimization:
 *      x + 0   = x (0 + x = x)
 *      x - 0   = x
 *      x * 0   = 0
 *      x / 0   = undefined behavior(unfolded, left to IR as a unreachable code)
 *      x % 0   = same as above
 *      x & 0   = 0
 *      x ^ 0   = x
 *      x | 0   = x
 *      x << 0  = x
 *      x >> 0  = x
 * 
 *      x * 1   = x
 *      x / 1   = x
 *      x % 1   = 0
 *
 *      x % -1  = 0
 *      x & -1  = x
 *      x | -1  = -1
*/
void constantFolder::visitArithSingle(expression * __lval, literal_expr *__rval, char __op) {
    /* Now, lval is normal expression and rval is a literal. */
    if (int __val = std::stoi(__rval->name); __val == 0) {
        switch (__op) {
            /* Remain the same as lval. */
            case '+': case '-': case '|': case '^': case '<': case '>':
                return setReplace(__lval);
            case '/': case '%': /* Undefined behavior. */
                return warningDivideByZero(__lval, __op), setNotReplace();
            case '*': case '&': /* Always 0. */
                return setReplace(__rval);
        }
        runtime_assert(false, "wtf?");
        __builtin_unreachable();
    } else if (__val == 1) {
        switch (__op) {
            case '*': case '/': return setReplace(__lval);           // x
            case '%': __rval->name = "0"; return setReplace(__rval); // 0
        }
    } else if (__val == -1) {
        switch (__op) {
            case '&': return setReplace(__lval);    // x
            case '|': return setReplace(__rval);    // -1
            case '%': __rval->name = "0"; return setReplace(__rval); // 0
        }
    }

    return setNotReplace();
}

void constantFolder::visitShortCircuit(binary_expr *ctx, bool __is_or) {
    auto *__lval = dynamic_cast <literal_expr *> (ctx->lval);
    auto *__rval = dynamic_cast <literal_expr *> (ctx->rval);

    if (__lval && __rval) {
        bool __lhs = __lval->name == "true";
        bool __rhs = __rval->name == "true";
        __lval->name = (__lhs + __rhs + __is_or > 1) ? "true" : "false";
        return setReplace(__lval);
    } else if (__lval) {
        bool __lhs = __lval->name == "true";
        return (__is_or ^ __lhs) ? setReplace(ctx->rval) : setReplace(ctx->lval);
    } else if (__rval) {
        bool __rhs = __rval->name == "true";
        return (__is_or ^ __rhs) ? setReplace(ctx->lval) : setNotReplace();
    }
    runtime_assert(false, "myonmyonmyon?");
    __builtin_unreachable();
}


void constantFolder::visitCompare(binary_expr *ctx, int __op) {
    auto *__lval = dynamic_cast <literal_expr *> (ctx->lval);
    auto *__rval = dynamic_cast <literal_expr *> (ctx->rval);
    if (!__rval || !__lval) return setNotReplace();
    std::strong_ordering __cmp = std::strong_ordering::equal;

    switch (__lval->sort) {
        case __lval->NUMBER:
            __cmp = std::stoi(__lval->name) <=> std::stoi(__rval->name); break;
        case __lval->STRING:
            __cmp = __lval->name <=> __rval->name; break;
        case __lval->_BOOL_:
            __cmp = (__lval->name == "true") <=> (__rval->name == "true"); break;
        case __lval->_NULL_:
            __cmp = std::strong_ordering::equal; break;
        default:
            __builtin_unreachable();
    }

    bool __result;
    switch (__op) {
        case 0: __result = __cmp != 0; break;
        case 1: __result = __cmp == 0; break;
        case 2: __result = __cmp < 0; break;
        case 3: __result = __cmp > 0; break;
        case 4: __result = __cmp <= 0; break;
        case 5: __result = __cmp >= 0; break;
        default: __builtin_unreachable();
    }

    __lval->type = ctx->type;   // Boolean type.
    __lval->sort = __lval->_BOOL_;
    __lval->name = __result ? "true" : "false";
    return setReplace(__lval);
}


void constantFolder::visitTernary(ternary_expr *ctx) {
    visitReplace(ctx->cond);
    visitReplace(ctx->lval);
    visitReplace(ctx->rval);

    if (auto *__cond = dynamic_cast <literal_expr *> (ctx->cond))
        return setReplace(__cond->name == "true" ? ctx->lval : ctx->rval);
    else
        return setNotReplace();
}


void constantFolder::visitMember(member_expr *ctx) {
    visitReplace(ctx->expr);
    return setNotReplace();
}

void constantFolder::visitConstruct(construct_expr *ctx) {
    for (auto &o : ctx->subscript) { visitReplace(o); }
    return setNotReplace();
}

void constantFolder::visitAtomic(atomic_expr *ctx) {
    return setNotReplace();
}

void constantFolder::visitLiteral(literal_expr *ctx) {
    return setNotReplace();
}

void constantFolder::visitFor(for_stmt *ctx) {
    if (ctx->init) visit(ctx->init);
    if (ctx->cond) visitReplace(ctx->cond);
    if (ctx->step) visitReplace(ctx->step);
    visit(ctx->body);
    return setNotReplace();
}

void constantFolder::visitWhile(while_stmt *ctx) {
    visitReplace(ctx->cond);
    visit(ctx->body);
    return setNotReplace();
}

void constantFolder::visitFlow(flow_stmt *ctx) {
    if (ctx->sort == ctx->RETURN && ctx->expr)
        visitReplace(ctx->expr);
    return setNotReplace();
}

void constantFolder::visitBlock(block_stmt *ctx) {
    for (auto *__p : ctx->stmt) visit(__p);
    return setNotReplace();
}
void constantFolder::visitBranch(branch_stmt *ctx) {
    for (auto &[__cond, __body] : ctx->branches) {
        visitReplace(__cond);
        visit(__body);
    }
    if (ctx->else_body) visit(ctx->else_body);
    return setNotReplace();
}

void constantFolder::visitSimple(simple_stmt *ctx) {
    for (auto &__p : ctx->expr) visitReplace(__p);
    return setNotReplace();
}

void constantFolder::visitVariableDef(variable_def *ctx) {
    for (auto &[__name, __init] : ctx->vars)
        if (__init) visitReplace(__init);
    return setNotReplace();
}

void constantFolder::visitFunctionDef(function_def *ctx) {
    visit(ctx->body);
    return setNotReplace();
}

void constantFolder::visitClassDef(class_def *ctx) {
    for (auto *__p : ctx->member) visit(__p);
    return setNotReplace();
}


} // namespace dark::AST
