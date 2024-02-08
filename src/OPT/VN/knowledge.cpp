#include "VN/gvn.h"
#include "IRnode.h"
#include <cmath>
#include <algorithm>


namespace dark::IR {
using ll = long long;

bool GlobalValueNumberPass::isEqual(definition *__lhs, definition *__rhs) { return __lhs == __rhs; }
bool GlobalValueNumberPass::isAbsLess(definition *__lhs, definition *__rhs) {
    auto __rsize = traceSize(__rhs);
    auto __sign  = std::signbit(__rsize.upper);
    if (__sign != std::signbit(__rsize.lower)) return false;
    auto __value = __sign ? -ll(__rsize.upper) : ll(__rsize.lower);
    auto __lsize = traceSize(__lhs);
    return __lsize.upper < __value && __lsize.lower > -__value;
}

sizeInfo GlobalValueNumberPass::traceSize(definition *__def) {
    if (auto *__literal = __def->as <literal> ()) {
        return sizeInfo { (int)__literal->to_integer() };
    } else if (auto *__tmp = __def->as <temporary> ()) {
        return infoMap[__tmp].size;
    } else {
        return sizeInfo {}; // Unknown. Consider the worst case.
    }
}

bitsInfo GlobalValueNumberPass::traceBits(definition *__def) {
    if (auto *__literal = __def->as <literal> ()) {
        return bitsInfo { (int)__literal->to_integer() };
    } else if (auto *__tmp = __def->as <temporary> ()) {
        return infoMap[__tmp].bits;
    } else {
        return bitsInfo {}; // Unknown. Consider the worst case.
    }
}

int GlobalValueNumberPass::traceSignBit(definition *__def) {
    if (auto *__literal = __def->as <literal> ()) {
        return __literal->to_integer() >= 0 ? 1 : -1;
    } else if (auto *__tmp = __def->as <temporary> ()) {
        auto __size = infoMap[__tmp].size;
        return 1 - (std::signbit(__size.upper) + std::signbit(__size.lower));
    } else {
        return 0; // Unknown. Consider the worst case.
    }
}

void GlobalValueNumberPass::buildKnowledge(binary_stmt *ctx) {
    switch (ctx->op) {
        case ctx->ADD: return updateAdd(ctx);
        case ctx->SUB: return updateSub(ctx);
        case ctx->MUL: return updateMul(ctx);
        case ctx->DIV: return updateDiv(ctx);
        case ctx->MOD: return updateMod(ctx);
        case ctx->SHL: return updateShl(ctx);
        case ctx->SHR: return updateShr(ctx);
        case ctx->AND: return updateAnd(ctx);
        case ctx->OR:  return updateOr(ctx);
        case ctx->XOR: return updateXor(ctx);
    }
}

void GlobalValueNumberPass::updateAdd(binary_stmt *ctx) {
    auto __lsize = traceSize(ctx->lval);
    auto __rsize = traceSize(ctx->rval);
    auto __size = sizeInfo {
        ll(__lsize.upper) + __rsize.upper,
        ll(__lsize.lower) + __rsize.lower
    };
    infoMap[ctx->dest].init(__size);
}

void GlobalValueNumberPass::updateSub(binary_stmt *ctx) {
    auto __lsize = traceSize(ctx->lval);
    auto __rsize = traceSize(ctx->rval);
    auto __size = sizeInfo {
        ll(__lsize.upper) - __rsize.lower,
        ll(__lsize.lower) - __rsize.upper
    };
    infoMap[ctx->dest].init(__size);
}

void GlobalValueNumberPass::updateMul(binary_stmt *ctx) {
    auto __lsize = traceSize(ctx->lval);
    auto __rsize = traceSize(ctx->rval);
    long long __array[] = {
        ll(__lsize.upper) * __rsize.lower,
        ll(__lsize.upper) * __rsize.upper,
        ll(__lsize.lower) * __rsize.lower,
        ll(__lsize.lower) * __rsize.upper
    };
    auto __size = sizeInfo {
        *std::ranges::max_element(__array, __array + 4),
        *std::ranges::min_element(__array, __array + 4)
    };
    infoMap[ctx->dest].init(__size);
}

static ll __maxAbs(ll __up, ll down) { return std::max(__up, -down); }


void GlobalValueNumberPass::updateDiv(binary_stmt *ctx) {
    auto __lsize = traceSize(ctx->lval);
    auto __rsize = traceSize(ctx->rval);

    if (std::signbit(__rsize.upper) == std::signbit(__rsize.lower)) {
        auto __bound = __maxAbs(__lsize.upper, __lsize.lower);
        return infoMap[ctx->dest].init(sizeInfo {__bound, -__bound});
    }

    /* Avoid divide by 0. */
    if (__rsize.lower == 0) __rsize.lower = 1;
    if (__rsize.upper == 0) __rsize.upper = 1;
    long long __array[] = {
        ll(__lsize.upper) / __rsize.lower,
        ll(__lsize.upper) / __rsize.upper,
        ll(__lsize.lower) / __rsize.lower,
        ll(__lsize.lower) / __rsize.upper
    };
    auto __size = sizeInfo {
        *std::ranges::max_element(__array, __array + 4),
        *std::ranges::min_element(__array, __array + 4)
    };
    infoMap[ctx->dest].init(__size);
}

void GlobalValueNumberPass::updateMod(binary_stmt *ctx) {
    auto __rsize = traceSize(ctx->rval);
    long long __bound = __maxAbs(__rsize.upper, __rsize.lower) - 1;
    if (__bound < 0) __bound = 0;
    infoMap[ctx->dest].init(sizeInfo {__bound, -__bound});
}

void GlobalValueNumberPass::updateShl(binary_stmt *ctx) {
    auto __lbits = traceBits(ctx->lval);
    auto __rsize = traceSize(ctx->rval);
    if (__rsize.upper > 0) {
        auto  __sum = ll(__lbits.top) + __rsize.upper;
        __lbits.top = __sum > 31 ? 31 : __sum;
    }
    if (__rsize.lower > 0) {
        auto  __sum = ll(__lbits.low) + __rsize.lower;
        __lbits.low = __sum > 31 ? 31 : __sum;
    }
    infoMap[ctx->dest].init(__lbits);
}

void GlobalValueNumberPass::updateShr(binary_stmt *ctx) {
    auto __lbits = traceBits(ctx->lval);
    auto __rsize = traceSize(ctx->rval);
    if (__rsize.lower > 0) {
        ll __sum = ll(__lbits.top) - __rsize.lower;
        __lbits.top = __sum < 0 ? 0 : __sum;
    }
    if (__rsize.upper > 0) {
        ll __sum = ll(__lbits.low) - __rsize.upper;
        __lbits.low = __sum < 0 ? 0 : __sum;
    }

    infoMap[ctx->dest].init(__lbits);
}

void GlobalValueNumberPass::updateAnd(binary_stmt *ctx) {
    auto __lbits = traceBits(ctx->lval);
    auto __rbits = traceBits(ctx->rval);
    auto __bits = bitsInfo {
        std::min(__lbits.top, __rbits.top),
        std::max(__lbits.low, __rbits.low)
    };

    infoMap[ctx->dest].init(__bits);
}

void GlobalValueNumberPass::updateOr(binary_stmt *ctx) {
    auto __lbits = traceBits(ctx->lval);
    auto __rbits = traceBits(ctx->rval);
    auto __bits = bitsInfo {
        std::max(__lbits.top, __rbits.top),
        std::min(__lbits.low, __rbits.low)
    };

    infoMap[ctx->dest].init(__bits);
}

void GlobalValueNumberPass::updateXor(binary_stmt *ctx) { return updateOr(ctx); }

void knowledge::init(sizeInfo __size) {
    size = __size;
    bits.low = 0;
    if (size.lower >= 0)
        bits.top = 31 - std::countl_zero((unsigned)size.upper);
}

void knowledge::init(bitsInfo __bits) {
    bits = __bits;
    if (bits.top != 31) {
        size.upper = (1 << (bits.top + 1)) - 1;
        size.lower = 1 << bits.low;
    } else {
        size.upper = INT32_MAX;
        size.lower = INT32_MIN;
    }
}


} // namespace dark::IR
