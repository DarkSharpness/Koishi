#include "VN/algebraic.h"
#include "IRnode.h"
#include <cstdint>

namespace dark::IR {

using ll = int64_t;

static constexpr std::hash <void *> __hash {};
static constexpr auto __create_int  = IRpool::create_integer;
static constexpr auto __create_bool = IRpool::create_boolean;

static binary_stmt *__get_binary(definition *__def) {
    auto __tmp = __def->as <temporary> ();
    if (!__tmp) return nullptr;
    return __tmp->def->as <binary_stmt> ();
}

static compare_stmt *__get_compare(definition *__def) {
    auto __tmp = __def->as <temporary> ();
    if (!__tmp) return nullptr;
    return __tmp->def->as <compare_stmt> ();
}

using _Comp_t = compare_stmt::compare_op;

static constexpr _Comp_t __swapOp(unsigned __op) {
    return static_cast <compare_stmt::compare_op> (__op < 2 ? __op : __op % 4 + 2);
}
static constexpr _Comp_t __reverseOp(unsigned __op) {
    return static_cast <compare_stmt::compare_op> (__op < 2 ? 1 - __op : 7 - __op);
}

static_assert (__swapOp(_Comp_t::EQ) == _Comp_t::EQ);
static_assert (__swapOp(_Comp_t::NE) == _Comp_t::NE);
static_assert (__swapOp(_Comp_t::LE) == _Comp_t::GE);
static_assert (__swapOp(_Comp_t::GE) == _Comp_t::LE);
static_assert (__swapOp(_Comp_t::LT) == _Comp_t::GT);
static_assert (__swapOp(_Comp_t::GT) == _Comp_t::LT);

static_assert (__reverseOp(_Comp_t::EQ) == _Comp_t::NE);
static_assert (__reverseOp(_Comp_t::NE) == _Comp_t::EQ);
static_assert (__reverseOp(_Comp_t::LE) == _Comp_t::GT);
static_assert (__reverseOp(_Comp_t::GE) == _Comp_t::LT);
static_assert (__reverseOp(_Comp_t::LT) == _Comp_t::GE);
static_assert (__reverseOp(_Comp_t::GT) == _Comp_t::LE);

/**
 * @brief Swap the lhs and rhs of the compare statement
 */
static void __swapCompare(compare_stmt *ctx) {
    std::swap(ctx->lval, ctx->rval);
    ctx->op = __swapOp(ctx->op);
}

/**
 * @return Whether the compare is a not expression.
 */
static bool __isNot(compare_stmt *ctx) {
    return ctx->op == ctx->NE
        && ctx->rval == IRpool::__false__;
}

void algebraicSimplifier::updateCompare(compare_stmt *ctx) {
    return;
    auto __type = ctx->lval->get_value_type();
    if (__type == typeinfo {int_type::ptr()}) {
        return compareInteger(ctx);
    } else if (__type == typeinfo {bool_type::ptr()}) {
        return compareBoolean(ctx);
    }
}


/* bool != 0  <=> bool  */
/* bool == 1  <=> bool  */
/* bool == 0  <=> !bool */
/* bool != 1  <=> !bool */
void algebraicSimplifier::compareBoolean(compare_stmt *ctx) {
    if (ctx->lval->as <boolean_constant> ())
        std::swap(ctx->lval, ctx->rval);

    if (auto __rhs = ctx->rval->as <boolean_constant> ()) {
        if (__rhs->value == true)
            ctx->rval = IRpool::__false__,
            ctx->op   = __reverseOp(ctx->op);

        // Now, it's a formatized boolean compare statement
        if (ctx->op == ctx->NE) return setResult(ctx->lval);

        auto __lcmp = __get_compare(ctx->lval);
        if (!__lcmp) return;

        ctx->op   = __reverseOp(__lcmp->op);
        ctx->lval = __lcmp->lval;
        ctx->rval = __lcmp->rval;
        return updateCompare(ctx);
    }

    if (__hash(ctx->lval) > __hash(ctx->rval))
        std::swap(ctx->lval, ctx->rval);
}


/* x + y op x + z   <=> x op z     */
/* x - y op x - z   <=> z op y     */
/* y - x op z - x   <=> y op z     */
/* x * c op y * c   <=> x op y (or y op x) */
void algebraicSimplifier::compareInteger(compare_stmt *ctx) {
    if (ctx->lval->as <integer_constant> ())
        std::swap(ctx->lval, ctx->rval),
        ctx->op = __swapOp(ctx->op);

    if (auto __rhs = ctx->rval->as <integer_constant> ()) {
        if (ctx->op == ctx->LE) {
            if (__rhs->value == INT32_MAX)
                return setResult(IRpool::__true__);
            ctx->op = ctx->LT;
            ctx->rval = __rhs = __create_int(__rhs->value + 1);
        } else if (ctx->op == ctx->GT) {
            if (__rhs->value == INT32_MAX)
                return setResult(IRpool::__false__);
            ctx->op = ctx->GE;
            ctx->rval = __rhs = __create_int(__rhs->value + 1);
        }

        if (__rhs->value == INT32_MIN) {
            if (ctx->op == ctx->LT) return setResult(IRpool::__false__);
            if (ctx->op == ctx->GE) return setResult(IRpool::__true__);
            runtime_assert(false, "wtf");
        }

        // op : ==, != , < , >=

        if (auto __lhs = ctx->lval->as <integer_constant> ()) {
            switch (ctx->op) {
                case ctx->EQ: return setResult(__create_bool(__lhs->value == __rhs->value));
                case ctx->NE: return setResult(__create_bool(__lhs->value != __rhs->value));
                case ctx->LT: return setResult(__create_bool(__lhs->value <  __rhs->value));
                case ctx->GE: return setResult(__create_bool(__lhs->value >= __rhs->value));
            } __builtin_unreachable();
        }

        auto __lbin = __get_binary(ctx->lval);
        if (!__lbin) return;

        // Only optimize those used once!
        // Optimization on some temporary is better.
        if (useMap[__lbin->dest].size() > 1) return;
        if (auto __rval = __lbin->rval->as <integer_constant> ()) {
            if (__lbin->op == __lbin->ADD) {
                /* x + c op d <=> x  */
                ctx->lval = __lbin->lval;
                ctx->rval = __create_int(__rhs->value - __rval->value);
                return compareInteger(ctx);
            } else if (__lbin->op == __lbin->MUL) {
                /* x * c op d <=> x op d / c */
                ctx->lval = __lbin->lval;
                ctx->rval = __create_int(__rhs->value / __rval->value);
                if (__rval->value < 0) ctx->op = __swapOp(ctx->op);
                return compareInteger(ctx);
            } else if (__lbin->op == __lbin->DIV) {
                /* x / c op d <=> x op c * d */
                const auto __tmp = ll(__rhs->value) * __rval->value;

                if (__tmp > INT32_MAX) {
                    if (ctx->op == ctx->GE || ctx->op == ctx->EQ)
                        return setResult(IRpool::__false__);
                    else // LT , NE
                        return setResult(IRpool::__true__);
                }

                if (__tmp < INT32_MIN) {
                    if (ctx->op == ctx->LT || ctx->op == ctx->EQ)
                        return setResult(IRpool::__false__);
                    else // GE , NE
                        return setResult(IRpool::__true__);
                }

                ctx->lval = __lbin->lval;
                ctx->rval = __create_int(__tmp);
                if (__rval->value < 0) ctx->op = __swapOp(ctx->op);
                return compareInteger(ctx);
            } else if (__lbin->op == __lbin->XOR) {
                /* x ^ c == d <=> x op c * d */
                ctx->lval = __lbin->lval;
                ctx->rval = __create_int(__rhs->value ^ __rval->value);
                return compareInteger(ctx);
            }
        }

        /* c - x op d <=> c - d op x  */
        if (auto __lval = __lbin->lval->as <integer_constant> ()) {
            if (__lbin->op == __lbin->SUB) {
                ctx->lval = __lbin->rval;
                ctx->rval = __create_int(__lval->value - __rhs->value);
                return compareInteger(ctx);
            }
        }

        return; // Fail to optimize.
    }

    if (__hash(ctx->lval) > __hash(ctx->rval)) {
        std::swap(ctx->lval, ctx->rval);
        ctx->op = __swapOp(ctx->op);
    }
}

/* Collect the use count of the non-literals. */
void algebraicSimplifier::collectUse(function *__func) {
    auto &&__operation = [this](statement *__node) {
        for (auto __use : __node->get_use())
            if (auto __tmp = __use->as <temporary> ())
                useMap[__tmp].push_back(__node);
    };
    for (auto __block : __func->data) visitBlock(__block, __operation);
}



} // namespace dark::IR
