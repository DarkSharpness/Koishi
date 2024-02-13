#include "VN/gvn.h"
#include "IRnode.h"
#include <cmath>

namespace dark::IR {

using ll = long long;

static binary_stmt *__get_binary(definition *__def) {
    auto __tmp = __def->as <temporary> ();
    if (!__tmp) return nullptr;
    return __tmp->def->as <binary_stmt> ();
}
static bool __is_negative(binary_stmt *__stmt) {
    return __stmt->op == binary_stmt::SUB
        && __stmt->lval == IRpool::__zero__;
}
integer_constant *__get_lconst(binary_stmt *__stmt) {
    return __stmt->lval->as <integer_constant> ();
}
integer_constant *__get_rconst(binary_stmt *__stmt) {
    return __stmt->rval->as <integer_constant> ();
}


static constexpr std::hash <void *> __hash {};
static constexpr auto __create_int = IRpool::create_integer;


/* Swap to right as much as possible. */
static inline void __formatize(definition *&__lval, definition *&__rval) {
    if (__lval->as <literal> ()) return std::swap(__lval, __rval);
    if (__rval->as <literal> ()) return; // Do nothing.
    if (__hash(__lval) > __hash(__rval)) std::swap(__lval, __rval);
}


void GlobalValueNumberPass::visitBinary(binary_stmt *ctx) {
    /**
     * This is intended to hold the old value.
     * We may not necessarily update the value of the expression
     * with the new value, since it may worsen the register
     * allocation pressure afterwards.
    */
    nodeMap.try_emplace(ctx, ctx);
    auto __lval = getValue(ctx->lval);
    auto __rval = getValue(ctx->rval);
    /* Try to simplify the expression first. */
    switch (ctx->op) {
        case ctx->ADD: visitAdd(ctx, __lval, __rval); break;
        case ctx->SUB: visitSub(ctx, __lval, __rval); break;
        case ctx->MUL: visitMul(ctx, __lval, __rval); break;
        case ctx->DIV: visitDiv(ctx, __lval, __rval); break;
        case ctx->MOD: visitMod(ctx, __lval, __rval); break;
        case ctx->SHL: visitShl(ctx, __lval, __rval); break;
        case ctx->SHR: visitShr(ctx, __lval, __rval); break;
        case ctx->AND: visitAnd(ctx, __lval, __rval); break;
        case ctx->OR:  visitOr(ctx, __lval, __rval);  break;
        case ctx->XOR: visitXor(ctx, __lval, __rval); break;
    }

    if (result != nullptr) { /* Equal to another value. */
        defMap[ctx->dest] = result;
        result = nullptr;
    } else { /* Tries to register a value, perform global value numerbing. */
        auto [__iter, __success] = exprMap.try_emplace(ctx, ctx->dest);
        defMap[ctx->dest] = __iter->second;
        // Insert success if and only if defMap[ctx->dest] = ctx->dest
        // Then, in removeHash, we need to remove {false, ctx->op, ctx->lval, ctx->rval}
    }
}

void GlobalValueNumberPass::visitAdd
    (binary_stmt *ctx, definition *__lval, definition *__rval) {
    __formatize(__lval, __rval);

    ctx->op = ctx->ADD;
    ctx->lval = __lval;
    ctx->rval = __rval;

    /* x + x = x * 2 */
    if (__lval == __rval)
        return visitMul(ctx, __lval, __create_int(2));

    auto *__lbin = __get_binary(__lval);
    auto *__rbin = __get_binary(__rval);
    auto *__rconst = __rval->as <integer_constant> ();

    if (__rbin) {
        if (__rbin->op == ctx->SUB) {
            /* x + (0 - y) = x - y */
            if (__rbin->lval == IRpool::__zero__)
                return visitSub(ctx, __lval, __rbin->rval);

            /* y + (x - y) = x */
            if (__rbin->rval == __lval)
                return setResult(__rbin->lval);
        } else if (__rbin->op == ctx->MUL) {
            auto __val = __get_rconst(__rbin);
            /* x + (x * c) = x * (c + 1) */
            if (__val && __rbin->lval == __lval)
                return visitMul(ctx, __lval, __create_int(__val->value + 1));

        }
    } else if (__rconst) {
        /* x + 0 = x */
        if (__rconst->value == 0) return setResult(__lval);
        /* c + d = (c + d) */
        if (auto __lconst = __lval->as <integer_constant> ())
            return setResult(__create_int(__lconst->value + __rconst->value));
    }

    if (__lbin) {
        if (__lbin->op == ctx->ADD) {
            /* (x + c) + d = x + (c + d) */
            if (auto __val = __get_rconst(__lbin); __val && __rconst)
                return visitAdd(ctx, __lbin->lval, __create_int(__val->value + __rconst->value));
        } else if (__lbin->op == ctx->SUB) {
            /* (0 - x) + y = y - x */
            if (__lbin->lval == IRpool::__zero__)
                return visitSub(ctx, __rval, __lbin->rval);

            /* (x - y) + y = x */
            if (__lbin->rval == __rval)
                return setResult(__lbin->lval);

            /* (c - x) + d = (c + d) - x */
            if (auto __val = __get_lconst(__lbin); __val && __rconst)
                return visitSub(ctx, __create_int(__val->value + __rconst->value), __lbin->rval);

        } else if (__lbin->op == ctx->MUL) {
            auto __val = __get_rconst(__lbin);
            /* x * c + x = x * (c + 1) */
            if (__val && __lbin->lval == __rval)
                return visitMul(ctx, __rval, __create_int(__val->value + 1));

        }
    }
}

void GlobalValueNumberPass::visitSub
    (binary_stmt *ctx, definition *__lval, definition *__rval) {
    if (auto __lit = __rval->as <integer_constant> ())
        return visitAdd(ctx, __lval, __create_int(-__lit->value));
    if (__lval == IRpool::__zero__)
        return visitNeg(ctx, __rval);

    ctx->op = ctx->SUB;
    ctx->lval = __lval;
    ctx->rval = __rval;

    /* x - x = 0 */
    if (__lval == __rval) return setResult(IRpool::__zero__);

    auto *__lbin = __get_binary(__lval);
    auto *__rbin = __get_binary(__rval);
    auto *__lconst = __lval->as <integer_constant> ();

    if (__rbin) {
        if (__rbin->op == ctx->ADD) {
            /* x - (x + y) = (0 - y) */
            if (__rbin->lval == __lval) return visitNeg(ctx, __rbin->rval);
            if (__rbin->rval == __lval) return visitNeg(ctx, __rbin->lval);

            /* c - (y + d) = (c - d) - y */
            if (auto __val = __get_rconst(__rbin); __val && __lconst)
                return visitSub(ctx, __create_int(__lconst->value - __val->value), __rbin->rval);

        } else if (__rbin->op == ctx->SUB) {
            /* x - (x - y) = y */
            if (__rbin->lval == __lval)
                return setResult(__rbin->rval);

            /* x - (0 - y) = y + x */
            if (__rbin->lval == IRpool::__zero__)
                return visitAdd(ctx, __rbin->rval, __lval);

            /* c - (d - y) = y + (c - d)  */
            if (auto __val = __get_lconst(__rbin); __val && __lconst)
                return visitAdd(ctx, __rbin->rval, __create_int(__lconst->value - __val->value));

        } else if (__rbin->op == ctx->MUL) {
            /* x - (x * c) = x * (1 - c) */
            auto __val = __get_rconst(__rbin);
            if (__val && __rbin->lval == __lval)
                return visitMul(ctx, __lval, __create_int(1 - __val->value));

        }
    }

    if (__lbin) {
        if (__lbin->op == ctx->ADD) {
            /* (x + y) - x = y */
            if (__lbin->lval == __rval) return setResult(__lbin->rval);
            if (__lbin->rval == __rval) return setResult(__lbin->lval);
        } else if (__lbin->op == ctx->SUB) {
            /* (x - y) - x = 0 - y */
            if (__lbin->rval == __rval)
                return visitNeg(ctx, __lbin->lval);

            /* (0 - x) - x = x * -2 */
            if (__lbin->lval == IRpool::__zero__ && __lbin->rval == __rval)
                return visitMul(ctx, __rval, __create_int(-2));

        } else if (__lbin->op == ctx->MUL) {
            /* x * c - x = x * (c - 1) */
            auto __val = __get_rconst(__lbin);
            if (__val && __lbin->lval == __rval)
                return visitMul(ctx, __rval, __create_int(__val->value - 1));

        }
    }
}

void GlobalValueNumberPass::visitNeg(binary_stmt *ctx, definition *__rval) {
    ctx->op = ctx->SUB;
    ctx->lval = IRpool::__zero__;
    ctx->rval = __rval;

    if (auto __rconst = __rval->as <integer_constant> ())
        return setResult(__create_int(-__rconst->value));
    if (auto __bin = __get_binary(__rval)) {
        if (__bin->op == ctx->ADD) {
            /* 0 - (x + c) = -c - x */
            if (auto __val = __get_rconst(__bin))
                return visitSub(ctx, __create_int(-__val->value), __bin->lval);
        } if (__bin->op == ctx->SUB) {
            /* 0 - (x - y) = y - x */
            return visitSub(ctx, __bin->rval, __bin->lval);
        } else if (__bin->op == ctx->MUL) {
            /* 0 - (x * c) = x * -c */
            if (auto __val = __get_rconst(__bin))
                return visitMul(ctx, __bin->lval, __create_int(-__val->value));
        } else if (__bin->op == ctx->SHL) {
            /* 0 - (c << x) = (-c) << x */
            if (auto __val = __get_lconst(__bin))
                return visitShl(ctx, __create_int(-__val->value), __bin->rval);
        }
    }
}


void GlobalValueNumberPass::visitMul
    (binary_stmt *ctx, definition *__lval, definition *__rval) {
    __formatize(__lval, __rval);

    ctx->op = ctx->MUL;
    ctx->lval = __lval;
    ctx->rval = __rval;

    auto *__lbin = __get_binary(__lval);
    auto *__rbin = __get_binary(__rval);
    auto *__rconst = __rval->as <integer_constant> ();

    if (__rconst) {
        /* c * d = (c * d) */
        if (auto *__lconst = __lval->as <integer_constant> ())
            return setResult(__create_int(__lconst->value * __rconst->value));
        /* x * 0 = 0 */
        if (__rconst->value == 0)   return setResult(IRpool::__zero__);
        /* x * 1 = x */
        if (__rconst->value == 1)   return setResult(__lval);
        /* x * -1 = -x */
        if (__rconst->value == -1)  return visitNeg(ctx, __lval);
    }

    if (__rconst && __lbin) {
        if (__lbin->op == ctx->MUL) {
            /* (x * c) * d = x * (c * d) */
            if (auto __val = __get_rconst(__lbin))
                return visitMul(ctx, __lbin->lval, __create_int(__val->value * __rconst->value));
        } else if (__lbin->op == ctx->SHL) {
            /* (x << c) * d = x * (c << d) */
            if (auto __val = __get_rconst(__lbin))
                return visitMul(ctx, __lbin->lval, __create_int(__val->value << __rconst->value));
            /* (c << x) * d = (c * d) << x */
            if (auto __val = __get_lconst(__lbin))
                return visitShl(ctx, __create_int(__val->value * __rconst->value), __lbin->rval);
        } else if (__lbin->op == ctx->SHR) {
            /* (x >> c) * (1 << c) = x & (-(1 << c)) */
            if (auto __val = __get_rconst(__lbin); 1 << __val->value == __rconst->value)
                return visitAnd(ctx, __lbin->lval, __create_int(-__rconst->value));
        } else if (__is_negative(__lbin)) {
            /* (0 - x) * c = x * (-c) */
            return visitMul(ctx, __lbin->rval, __create_int(-__rconst->value));
        }
    } else if (__rbin && __lbin) {
        /* (0 - x) * (0 - y) = x * y */
        if (__is_negative(__lbin) && __is_negative(__rbin))
            return visitMul(ctx, __lbin->rval, __rbin->rval);
    }
}


void GlobalValueNumberPass::visitDiv
    (binary_stmt *ctx, definition *__lval, definition *__rval) {
    /* Undefined behavior. */
    if (__rval == IRpool::__zero__) return setResult(IRpool::create_undefined({},1));

    ctx->op = ctx->DIV;
    ctx->lval = __lval;
    ctx->rval = __rval;

    if (__lval == __rval) return setResult(IRpool::__pos1__);
    // if (isAbsLess(__lval, __rval)) return setResult(IRpool::__zero__);

    auto *__lbin = __get_binary(__lval);
    auto *__rbin = __get_binary(__rval);
    auto *__rconst = __rval->as <integer_constant> ();

    if (__rconst) {
        /* c / d = (c / d) */
        if (auto *__lconst = __lval->as <integer_constant> ())
            return setResult(__create_int(__lconst->value / __rconst->value));

        /* x / 1 = x */
        if (__rconst->value == 1)
            return setResult(__lval);

        /* x / -1 = 0 - x */
        if (__rconst->value == -1)
            return visitNeg(ctx, __lval);

        /* x / (1 << c) = x >> c (when x >= 0) */
        // if (std::has_single_bit((unsigned)__rconst->value))
        //     if (traceSignBit(__lval) > 0)
        //         return visitShr(ctx, __lval,
        //             __create_int(std::countr_zero((unsigned)__rconst->value)));

    } else if (__rbin && __is_negative(__rbin)) {
        /* x / (0 - x) = -1 */
        if (__lval == __rbin->rval)
            return setResult(IRpool::__neg1__);

        /* c / (0 - x) = -c / x */
        if (auto __lconst = __lval->as <integer_constant> ())
            return visitDiv(ctx, __create_int(-__lconst->value), __rbin->rval);

        /* (0 - x) / (0 - y) = x / y */
        if (__lbin && __is_negative(__lbin))
            return visitDiv(ctx, __lbin->rval, __rbin->rval);
    }

    if (__lbin && __rconst) {
        if (__lbin->op == ctx->MUL) {
            /* (x * c) / d = x * (c / d) (when c % d == 0)*/
            if (auto __val = __get_rconst(__lbin);
                __val && __val->value % __rconst->value == 0)
                return visitMul(ctx, __lbin->lval, __create_int(__val->value / __rconst->value));
        } else if (__lbin->op == ctx->DIV) {
            /* (x / c) / d = x / (c * d) */
            if (auto __val = __get_rconst(__lbin))
                return visitDiv(ctx, __lbin->lval, __create_int(__val->value * __rconst->value));
        } else if (__is_negative(__lbin)) {
            /* (0 - x) / c = x / (-c) */
            return visitDiv(ctx, __lbin->rval, __create_int(-__rconst->value));
        }
    }

    if (__lbin) {
        if (__lbin->op == ctx->MUL) {
            /* (x * y) / y = x */
            if (__lbin->rval == __rval) return setResult(__lbin->lval);
            if (__lbin->lval == __rval) return setResult(__lbin->rval);
        } else if (__lbin->rval == __rval && __is_negative(__lbin)) {
            /* (0 - x) / x = -1 */
            return setResult(IRpool::__neg1__);
        }
    }
}


void GlobalValueNumberPass::visitMod
    (binary_stmt *ctx, definition *__lval, definition *__rval) {
    /* Undefined behavior. */
    if (__rval == IRpool::__zero__) return setResult(IRpool::create_undefined({},1));

    ctx->op = ctx->MOD;
    ctx->lval = __lval;
    ctx->rval = __rval;

    if (__lval == __rval) return setResult(IRpool::__zero__);
    // if (isAbsLess(__lval, __rval)) return setResult(__lval);

    auto *__lbin = __get_binary(__lval);
    auto *__rconst = __rval->as <integer_constant> ();

    if (__rconst) {
        if (__rconst->value < 0) // Formatize the expression.
            ctx->rval = __rval = __rconst = __create_int(-__rconst->value);

        /* x % +-1 = 0 */
        if (__rconst->value == 1 || __rconst->value == -1)
            return setResult(IRpool::__zero__);
        /* c % d = (c % d) */
        if (auto __lconst = __lval->as <integer_constant> ())
            return setResult(__create_int(__lconst->value % __rconst->value));

        /* x % (1 << c) = x & ((1 << c) - 1) (when x >= 0) */
        // if (std::has_single_bit((unsigned)__rconst->value))
        //     if (traceSignBit(__lval) > 0)
        //         return visitAnd(ctx, __lval, __create_int(__rconst->value - 1));

    }

    if (__lbin) {
        if (__lbin->op == ctx->MUL) {
            /* (x * y) % y = 0 */
            if (__lbin->rval == __rval) return setResult(IRpool::__zero__);
            if (__lbin->lval == __rval) return setResult(IRpool::__zero__);
            /* (x * c) % d = 0 */
            if (auto __val = __get_rconst(__lbin);
                __val && __rconst && __val->value % __rconst->value == 0)
                return setResult(IRpool::__zero__);
        } else if (__lbin->rval == __rval && __is_negative(__lbin)) {
            /* (0 - x) % x = 0 */
            return setResult(IRpool::__zero__);
        }
    }

    /* x % (0 - y) = x % y */
    if (auto *__rbin = __get_binary(__rval); __rbin && __is_negative(__rbin))
        return visitMod(ctx, __lbin->rval, __rbin->rval);
}

void GlobalValueNumberPass::visitShl
    (binary_stmt *ctx, definition *__lval, definition *__rval) {
    ctx->op = ctx->SHL;
    ctx->lval = __lval;
    ctx->rval = __rval;

    if (__lval == IRpool::__zero__) return setResult(__lval);

    auto __rconst = __rval->as <integer_constant> ();
    if (!__rconst) return; // Cannot simplify.

    auto __shift = __rconst->value; // Must be non-negative.

    if (__shift < 0) return setResult(IRpool::create_undefined({},1));
    if (__shift == 0) return setResult(__lval);
    if (__shift > 31) return setResult(IRpool::__zero__);

    /* c << d = (c << d)*/
    if (auto __lconst = __lval->as <integer_constant> ())
        return setResult(__create_int(__lconst->value << __shift));

    if (auto __lbin = __get_binary(__lval)) {
        if (__lbin->op == ctx->SHL) {
            /* (c << x) << d = (c << d) << x */
            if (auto __val = __get_lconst(__lbin))
                return visitShl(ctx, __create_int(__val->value << __shift), __lbin->rval);
            /* (x << c) << d = x << (c + d) */
            if (auto __val = __get_rconst(__lbin))
                return visitShl(ctx, __lbin->lval, __create_int(__val->value + __shift));
        } else if (__lbin->op == ctx->SHR) {
            /* (x >> c) << c = x & -(1 << c) */
            if (auto __val = __get_rconst(__lbin); __val->value == __shift)
                return visitAnd(ctx, __lbin->lval, __create_int(-(1 << __shift)));
        }
    }

    // auto __type = traceBits(__lval);
    // /* x << c = 0 (all overflowed) */
    // if (__type.low > 31 - __shift) return setResult(IRpool::__zero__);
    // /* x << c = x * (1 << c) (no overflow) */
    // if (__type.top < 31 - __shift) return visitMul(ctx, __lval, __create_int(1 << __shift));
}

void GlobalValueNumberPass::visitShr
    (binary_stmt *ctx, definition *__lval, definition *__rval) {
    ctx->op = ctx->SHR;
    ctx->lval = __lval;
    ctx->rval = __rval;

    if (__lval == IRpool::__zero__) return setResult(__lval);

    auto __rconst = __rval->as <integer_constant> ();
    if (!__rconst) return; // Cannot simplify.

    auto __shift = __rconst->value; // Must be non-negative.
    if (__shift < 0) return setResult(IRpool::create_undefined({},1));
    if (__shift == 0) return setResult(__lval);

    /* c >> d = (c >> d) */
    if (auto __lconst = __lval->as <integer_constant> ())
        return setResult(__create_int(__lconst->value >> __shift));

    if (auto __lbin = __get_binary(__lval)) {
        if (__lbin->op == ctx->SHR) {
            /* (c >> x) >> d = (c >> d) >> x */
            if (auto __val = __get_lconst(__lbin))
                return visitShr(ctx, __create_int(__val->value >> __shift), __lbin->rval);
            /* (x >> c) >> d = x >> (c + d) */
            if (auto __val = __get_rconst(__lbin))
                return visitShr(ctx, __lbin->lval, __create_int(__val->value + __shift));
        }
    }

    /* x >> c = 0 (all underflowed, and x >= 0) */
    // auto __type = traceBits(__lval);
    // if (__type.top < 31 && __type.top < __shift)
    //     return setResult(IRpool::__zero__);
}


void GlobalValueNumberPass::visitAnd
    (binary_stmt *ctx, definition *__lval, definition *__rval) {
    __formatize(__lval, __rval);

    ctx->op = ctx->AND;
    ctx->lval = __lval;
    ctx->rval = __rval;

    /* x & x = x */
    if (__lval == __rval) return setResult(__lval);

    if (auto *__rconst = __rval->as <integer_constant> ()) {
        /* x & 0 = 0 */
        if (__rconst->value == 0) return setResult(__rconst);
        /* x & -1 = x */
        if (__rconst->value == -1) return setResult(__lval);
        /* c & d = (c & d) */
        if (auto __lconst = __lval->as <integer_constant> ())
            return setResult(__create_int(__lconst->value & __rconst->value));
    } else if (auto *__rbin = __get_binary(__rval)) {
        if (__rbin->op == ctx->AND) {
            /* x & (x & y) = x & y */
            if (__rbin->lval == __lval) return setResult(__rval);
            if (__rbin->rval == __lval) return setResult(__rval);
        } else if (__rbin->op == ctx->OR) {
            /* x & (x | y) = x */
            if (__rbin->lval == __lval) return setResult(__lval);
            if (__rbin->rval == __lval) return setResult(__lval);
        }
    }

    if (auto *__lbin = __get_binary(__lval)) {
        if (__lbin->op == ctx->AND) {
            /* (x & y) & x = x & y */
            if (__lbin->lval == __rval) return setResult(__lval);
            if (__lbin->rval == __rval) return setResult(__lval);
        } else if (__lbin->op == ctx->OR) {
            /* (x | y) & x = x */
            if (__lbin->lval == __rval) return setResult(__rval);
            if (__lbin->rval == __rval) return setResult(__rval);
        }
    }

    // auto __lbit = traceBits(__lval);
    // auto __rbit = traceBits(__rval);
    // /* No bit intersection. */
    // if (__lbit.top < __rbit.low || __rbit.top < __lbit.low)
    //      return setResult(IRpool::__zero__);
}


void GlobalValueNumberPass::visitOr
    (binary_stmt *ctx, definition *__lval, definition *__rval) {
    /* First, formatize the expression. */
    __formatize(__lval, __rval);

    ctx->op = ctx->OR;
    ctx->lval = __lval;
    ctx->rval = __rval;

    /* x | x = x */
    if (__lval == __rval) return setResult(__lval);

    if (auto *__rconst = __rval->as <integer_constant> ()) {
        /* x | 0 = x */
        if (__rconst->value == 0) return setResult(__lval);
        /* x | -1 = -1 */
        if (__rconst->value == -1) return setResult(__rconst);
        /* c | d = (c | d) */
        if (auto __lconst = __lval->as <integer_constant> ())
            return setResult(__create_int(__lconst->value | __rconst->value));
    } else if (auto *__rbin = __get_binary(__rval)) {
        if (__rbin->op == ctx->AND) {
            /* x | (x & y) = x */
            if (__rbin->lval == __lval) return setResult(__lval);
            if (__rbin->rval == __lval) return setResult(__lval);
        } else if (__rbin->op == ctx->OR) {
            /* x | (x | y) = x | y */
            if (__rbin->lval == __lval) return setResult(__rval);
            if (__rbin->rval == __lval) return setResult(__rval);
        } else if (__rbin->op == ctx->XOR) {
            /* x | (x ^ y) = x | y */
            if (__rbin->lval == __lval) return visitOr(ctx, __lval, __rbin->rval);
            if (__rbin->rval == __lval) return visitOr(ctx, __lval, __rbin->lval);
        }
    }

    if (auto *__lbin = __get_binary(__lval)) {
        if (__lbin->op == ctx->AND) {
            /* (x & y) | x = x */
            if (__lbin->lval == __rval) return setResult(__rval);
            if (__lbin->rval == __rval) return setResult(__rval);
        } else if (__lbin->op == ctx->OR) {
            /* (x | y) | x = x | y */
            if (__lbin->lval == __rval) return setResult(__lval);
            if (__lbin->rval == __rval) return setResult(__lval);
        } else if (__lbin->op == ctx->XOR) {
            /* (x ^ y) | x = x | y */
            if (__lbin->lval == __rval) return visitOr(ctx, __rval, __lbin->rval);
            if (__lbin->rval == __rval) return visitOr(ctx, __rval, __lbin->lval);
        }
    }
}


void GlobalValueNumberPass::visitXor
    (binary_stmt *ctx, definition *__lval, definition *__rval) {
    /* First, formatize the expression. */
    __formatize(__lval, __rval);

    ctx->op = ctx->XOR;
    ctx->lval = __lval;
    ctx->rval = __rval;

    /* x ^ x = 0 */
    if (__lval == __rval) return setResult(IRpool::__zero__);

    if (auto *__rconst = __rval->as <integer_constant> ()) {
        /* x ^ 0 = x */
        if (__rconst->value == 0) return setResult(__lval);
        /* c ^ d = (c ^ d) */
        if (auto __lconst = __lval->as <integer_constant> ())
            return setResult(__create_int(__lconst->value ^ __rconst->value));
    } else if (auto *__rbin = __get_binary(__rval)) {
        if (__rbin->op == ctx->XOR) {
            /* x ^ (x ^ y) = y */
            if (__rbin->lval == __lval) return setResult(__rbin->rval);
            if (__rbin->rval == __lval) return setResult(__rbin->lval);
        }
    }

    if (auto *__lbin = __get_binary(__lval)) {
        if (__lbin->op == ctx->XOR) {
            /* (x ^ y) ^ x = y */
            if (__lbin->lval == __rval) return setResult(__lbin->rval);
            if (__lbin->rval == __rval) return setResult(__lbin->lval);
        }
    }
}




} // namespace dark::IR
