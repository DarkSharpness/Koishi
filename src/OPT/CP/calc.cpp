#include "CP/calc.h"
#include "IRnode.h"

namespace dark::IR {

[[noreturn]] static void __unreachable_assert() {
    runtime_assert(false, "Unreachable");
    __builtin_unreachable();
}

ConstantCalculator::ConstantCalculator(node *__stmt, const _List_t &__list, bool __hasDom)
: valueList(__list), hasDomTree(__hasDom) { visit(__stmt); }

void ConstantCalculator::visitCompare(compare_stmt *ctx) {
    runtime_assert(valueList.size() == 2, "Invalid argument length");
    auto *__lval = valueList[0];
    auto *__rval = valueList[1];
    if (__lval->as <undefined> ()) return setResult(__lval);
    if (__rval->as <undefined> ()) return setResult(__rval);

    if (__lval && __lval == __rval) {
        switch (ctx->op) {
            case ctx->EQ:
            case ctx->LE:
            case ctx->GE:
                return setResult(IRpool::__true__);
            case ctx->NE:
            case ctx->LT:
            case ctx->GT:
                return setResult(IRpool::__false__);
        }
        __unreachable_assert();
    }

    auto *__lhs = __lval->as <literal> ();
    auto *__rhs = __rval->as <literal> ();
    if (!__lhs || !__rhs) return;
    auto __l = __lhs->to_integer();
    auto __r = __rhs->to_integer();
    switch (ctx->op) {
        case ctx->EQ: return setResult(IRpool::create_boolean(__l == __r));
        case ctx->NE: return setResult(IRpool::create_boolean(__l != __r));
        case ctx->LT: return setResult(IRpool::create_boolean(__l <  __r));
        case ctx->LE: return setResult(IRpool::create_boolean(__l <= __r));
        case ctx->GT: return setResult(IRpool::create_boolean(__l >  __r));
        case ctx->GE: return setResult(IRpool::create_boolean(__l >= __r));
    }

    __unreachable_assert();
}

void ConstantCalculator::visitBinary(binary_stmt *ctx) {
    runtime_assert(valueList.size() == 2, "Invalid argument length");
    auto *__lval = valueList[0];
    auto *__rval = valueList[1];
    if (__lval->as <undefined> ()) return setResult(__lval);
    if (__rval->as <undefined> ()) return setResult(__rval);

    if (__rval == IRpool::__zero__) {
        switch (ctx->op) {
            case ctx->DIV: case ctx->MOD:
                return setResult(IRpool::create_undefined({}, 1));
            case ctx->ADD: case ctx->SUB:
            case ctx->SHL: case ctx->SHR:
            case ctx->XOR: case ctx->OR: 
                return setResult(__lval);
            case ctx->MUL: case ctx->AND:
                return setResult(__rval);
        }
        __unreachable_assert();
    } else if (__rval == IRpool::__pos1__) {
        switch (ctx->op) {
            case ctx->DIV: case ctx->MUL:   // x / 1 = x
                return setResult(__lval);   // x * 1 = x
            case ctx->MOD:                  // x % 1 = 0
                return setResult(IRpool::__zero__);
        }
    } else if (__rval == IRpool::__neg1__) {
        switch (ctx->op) {
            case ctx->AND: return setResult(__lval); // x & -1 = x
            case ctx->OR:  return setResult(__rval); // x | -1 = -1
        }
    }

    if (__lval == IRpool::__zero__) {
        switch (ctx->op) {
            case ctx->DIV: case ctx->MOD:
            case ctx->MUL: case ctx->AND:
            case ctx->SHL: case ctx->SHR:
                return setResult(__lval);
            case ctx->ADD:
            case ctx->XOR: case ctx->OR: 
                return setResult(__rval);
        }
        // Only case left: 0 - x
    } else if (__lval == IRpool::__pos1__) { // 1 * x
        switch (ctx->op) case ctx->MUL: return setResult(__rval);
    } else if (__lval == IRpool::__neg1__) {
        switch (ctx->op) {
            case ctx->AND: return setResult(__rval);
            case ctx->OR:  return setResult(__lval);
        }
    }

    if (__lval && __lval == __rval) {
        switch (ctx->op) {
            case ctx->XOR: case ctx->SUB:
            case ctx->MOD: case ctx->SHR:   // Special: x >> x = 0
                return setResult(IRpool::__zero__);
            case ctx->DIV:
                return setResult(IRpool::__pos1__);
            case ctx->AND: case ctx->OR:
                return setResult(__lval);
        }
    }

    auto *__lhs = __lval->as <integer_constant> ();
    auto *__rhs = __rval->as <integer_constant> ();
    if (!__lhs || !__rhs) return;
    auto __l = __lhs->to_integer();
    auto __r = __rhs->to_integer();

    switch (ctx->op) {
        case ctx->ADD: return setResult(IRpool::create_integer(__l + __r));
        case ctx->SUB: return setResult(IRpool::create_integer(__l - __r));
        case ctx->MUL: return setResult(IRpool::create_integer(__l * __r));
        case ctx->DIV: return setResult(IRpool::create_integer(__l / __r));
        case ctx->MOD: return setResult(IRpool::create_integer(__l % __r));
        case ctx->AND: return setResult(IRpool::create_integer(__l & __r));
        case ctx->OR:  return setResult(IRpool::create_integer(__l | __r));
        case ctx->XOR: return setResult(IRpool::create_integer(__l ^ __r));
        case ctx->SHL: return setResult(IRpool::create_integer(__l << __r));
        case ctx->SHR: return setResult(IRpool::create_integer(__l >> __r));
    }

    __unreachable_assert();
}

void ConstantCalculator::visitJump(jump_stmt *) {}
void ConstantCalculator::visitBranch(branch_stmt *) {}
void ConstantCalculator::visitCall(call_stmt *) {}
void ConstantCalculator::visitLoad(load_stmt *) {}
void ConstantCalculator::visitStore(store_stmt *) {}
void ConstantCalculator::visitReturn(return_stmt *) {}
void ConstantCalculator::visitGet(get_stmt *) {}
void ConstantCalculator::visitPhi(phi_stmt *ctx) {
    temporary  *__def = ctx->dest;
    definition *__tmp = nullptr;

    for(auto __use : valueList) {
        if (__use == nullptr) return;
        if (__use == __def) continue;
        /**
         * @brief Simple rule as below:
         * undef + undef = undef.
         * undef + def   = single phi def.
         * def   + def   = non-single phi.
        */
        if (__use->as <undefined> ()) continue;
        else if (__tmp == nullptr) __tmp = __use;
        /* Multiple definition: not a zero/ single phi! */
        else if (__tmp != __use) return;
    }

    /* This is on consideration of dominance relationship. */
    if (!hasDomTree && __tmp->as <temporary> ()) return;
    if (!__tmp) __tmp = IRpool::create_undefined(__def->type);
    return setResult(__tmp);
}
void ConstantCalculator::visitUnreachable(unreachable_stmt *) {}


} // namespace dark::IR
