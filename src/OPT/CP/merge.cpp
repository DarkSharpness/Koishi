#include "CP/sckp.h"
#include "IRnode.h"
#include <bit>
#include <cmath>
#include <ranges>
#include <algorithm>

namespace dark::IR {

static sizeInfo __merge(sizeInfo __lhs, sizeInfo __rhs) {
    return sizeInfo {
        std::min(__lhs.lower, __rhs.lower),
        std::max(__lhs.upper, __rhs.upper),
    };
}

static bitsInfo __merge(bitsInfo __lhs, bitsInfo __rhs) {
    return bitsInfo {
        __lhs.state,
        (__lhs.valid & __rhs.valid) & ~(__lhs.state ^ __rhs.state)
    };
}

using ll = int64_t;

static auto updateAdd(binary_stmt *ctx, sizeInfo __lhs, sizeInfo __rhs)
    -> sizeInfo {
    return sizeInfo { ll(__lhs.lower) + __rhs.lower, ll(__lhs.upper) + __rhs.upper, };
}

static auto updateSub(binary_stmt *ctx, sizeInfo __lhs, sizeInfo __rhs)
    -> sizeInfo {
    return sizeInfo { ll(__lhs.lower) - __rhs.upper, ll(__lhs.upper) - __rhs.lower, };
}

static auto updateMul(binary_stmt *ctx, sizeInfo __lhs, sizeInfo __rhs)
    -> sizeInfo {
    ll __array[] = {
        ll(__lhs.lower) * __rhs.lower,
        ll(__lhs.lower) * __rhs.upper,
        ll(__lhs.upper) * __rhs.lower,
        ll(__lhs.upper) * __rhs.upper,
    };
    return sizeInfo { std::ranges::min(__array), std::ranges::max(__array), };
}

static auto updateDiv(binary_stmt *ctx, sizeInfo __lhs, sizeInfo __rhs)
    -> sizeInfo {
    if (__rhs.lower < 0 && __rhs.upper > 0) {
        ll __abs = std::max(-ll(__lhs.lower), ll(__lhs.upper));
        return sizeInfo { -__abs, __abs };
    }

    if (__rhs.lower == 0)
        __rhs.lower = std::signbit(__rhs.upper) ? -1 : 1;
    if (__rhs.upper == 0)
        __rhs.upper = std::signbit(__rhs.lower) ? -1 : 1;

    ll __array[] = {
        ll(__lhs.lower) / __rhs.lower,
        ll(__lhs.lower) / __rhs.upper,
        ll(__lhs.upper) / __rhs.lower,
        ll(__lhs.upper) / __rhs.upper,
    };

    return sizeInfo { std::ranges::min(__array), std::ranges::max(__array), };
}

static auto updateMod(binary_stmt *ctx, sizeInfo __lhs, sizeInfo __rhs)
    -> sizeInfo {
    if (__lhs.lower == __lhs.upper && __rhs.lower == __rhs.upper) {
        if (!__rhs.lower) __rhs.lower = 1;
        return sizeInfo { __lhs.lower % __rhs.lower };
    }

    ll __abs = std::max(-ll(__rhs.lower), ll(__rhs.upper));
    ll __bound = __abs < 1 ? 0 : __abs - 1;
    return sizeInfo { -__bound, __bound};
}

static auto updateShl(binary_stmt *ctx, bitsInfo __lhs, sizeInfo __rhs)
    -> bitsInfo {
    auto [__l, __r] = __rhs;
    if (__l >= 32) return bitsInfo {0};
    if (__r <= 0 ) return __lhs;
    if (__l == __r) {
        __lhs.state <<= __l;
        __lhs.valid <<= __l;
        __lhs.valid |= (1u << __l) - 1; // Low bits are 0.
        return __lhs;
    } else return bitsInfo {}; // Too hard to trace.
}

static auto updateShr(binary_stmt *ctx, bitsInfo __lhs, sizeInfo __rhs)
    -> bitsInfo {
    auto [__l, __r] = __rhs;
    __l = (__l < 0) ? 0 : (__l > 31) ? 31 : __l;
    __r = (__r < 0) ? 0 : (__r > 31) ? 31 : __r;
    if (__l == __r) {
        __lhs.state >>= __l; // Note that state is a signed type.
        __lhs.valid >>= __l; // High bits are valid iff sign bit is valid.
        return __lhs;
    } else return bitsInfo {}; // Too hard to trace.
}

static auto updateAnd(binary_stmt *ctx, bitsInfo __lhs, bitsInfo __rhs)
    -> bitsInfo {
    return bitsInfo {
        __lhs.state & __rhs.state,
        // LHS valid 0 or RHS valid 0 or (LHS & RHS valid 1)
        ((~__lhs.state & __lhs.valid) | (~__rhs.state & __rhs.valid)) |
        (( __lhs.state & __lhs.valid) & ( __rhs.state & __rhs.valid))
    };
}

static auto updateOr(binary_stmt *ctx, bitsInfo __lhs, bitsInfo __rhs)
    -> bitsInfo {
    return bitsInfo {
        __lhs.state | __rhs.state,
        // LHS valid 1 or RHS valid 1 or (LHS & RHS valid 0)
        ((~__lhs.state & __lhs.valid) & (~__rhs.state & __rhs.valid)) |
        (( __lhs.state & __lhs.valid) | ( __rhs.state & __rhs.valid))
    };
}

static auto updateXor(binary_stmt *ctx, bitsInfo __lhs, bitsInfo __rhs)
    -> bitsInfo {
    return bitsInfo { __lhs.state ^ __rhs.state, __lhs.valid & __rhs.valid };
}

static int updateNotEqual(sizeInfo __lhs, sizeInfo __rhs) {
    if (__lhs.lower > __rhs.upper) return 1;
    if (__lhs.upper < __rhs.lower) return 1;
    if (__lhs.lower == __lhs.upper
    &&  __rhs.lower == __rhs.upper
    &&  __lhs.lower == __rhs.lower) return -1;
    return 0;
}

static int updateLessThan(sizeInfo __lhs, sizeInfo __rhs) {
    if (__lhs.lower > __rhs.upper) return -1;
    if (__lhs.upper < __rhs.lower) return 1;
    return 0;
}

static int updateLessEqual(sizeInfo __lhs, sizeInfo __rhs) {
    if (__lhs.lower >= __rhs.upper) return -1;
    if (__lhs.upper <= __rhs.lower) return 1;
    return 0;
}


void KnowledgePropagatior::visitCompare(compare_stmt *ctx) {
    auto &__def = defMap[ctx->dest];
    // No knowledge for non-integer type.
    if (ctx->lval->get_value_type() != typeinfo {int_type::ptr(),0})
        __def.type = defInfo::UNCERTAIN;
    // Uncertain are useless.
    if (__def.type == __def.UNCERTAIN) return;
    int __val {};
    switch (ctx->op) {
        case ctx->NE: __val = updateNotEqual(traceSize(ctx->lval), traceSize(ctx->rval)); break;
        case ctx->EQ: __val = -updateNotEqual(traceSize(ctx->lval), traceSize(ctx->rval)); break;
        case ctx->LT: __val = updateLessThan(traceSize(ctx->lval), traceSize(ctx->rval)); break;
        case ctx->LE: __val = updateLessEqual(traceSize(ctx->lval), traceSize(ctx->rval)); break;
        case ctx->GT: __val = -updateLessThan(traceSize(ctx->lval), traceSize(ctx->rval)); break;
        case ctx->GE: __val = -updateLessEqual(traceSize(ctx->lval), traceSize(ctx->rval)); break;
    }
    auto __size = __val ? sizeInfo{(__val + 1) >> 1} : sizeInfo {0, 1};
    return updateSize(ctx->dest, __size);
}

void KnowledgePropagatior::visitBinary(binary_stmt *ctx) {
    std::variant <sizeInfo, bitsInfo> __val;
    switch (ctx->op) {
        case ctx->ADD: __val = updateAdd(
            ctx, traceSize(ctx->lval), traceSize(ctx->rval)); break;
        case ctx->SUB: __val = updateSub(
            ctx, traceSize(ctx->lval), traceSize(ctx->rval)); break;
        case ctx->MUL: __val = updateMul(
            ctx, traceSize(ctx->lval), traceSize(ctx->rval)); break;
        case ctx->DIV: __val = updateDiv(
            ctx, traceSize(ctx->lval), traceSize(ctx->rval)); break;
        case ctx->MOD: __val = updateMod(
            ctx, traceSize(ctx->lval), traceSize(ctx->rval)); break;
        case ctx->SHL: __val = updateShl(
            ctx, traceBits(ctx->lval), traceSize(ctx->rval)); break;
        case ctx->SHR: __val = updateShr(
            ctx, traceBits(ctx->lval), traceSize(ctx->rval)); break;
        case ctx->AND: __val = updateAnd(
            ctx, traceBits(ctx->lval), traceBits(ctx->rval)); break;
        case ctx->OR:  __val = updateOr(
            ctx, traceBits(ctx->lval), traceBits(ctx->rval)); break;
        case ctx->XOR: __val = updateXor(
            ctx, traceBits(ctx->lval), traceBits(ctx->rval)); break;
    }
    if (__val.index() == 0) {
        updateSize(ctx->dest, std::get <0> (__val));
    } else {
        updateBits(ctx->dest, std::get <1> (__val));
    }
}

void KnowledgePropagatior::updateBits(temporary *__tmp, bitsInfo __bits) {
    auto &__def = defMap[__tmp];
    if (__def.type == __def.UNCERTAIN) return;
    if (__def.type != __def.UNDEFINED) {
        if (__def.bits <= __bits) return;
        __def.init(__merge(__def.bits, __bits));
    } else {
        __def.init(__bits);
    }
    __def.type++;
    updated = true;
}

void KnowledgePropagatior::updateSize(temporary *__tmp, sizeInfo __size) {
    auto &__def = defMap[__tmp];
    if (__def.type == __def.UNCERTAIN) return;
    if (__def.type != __def.UNDEFINED) {
        if (__def.size <= __size) return;
        __def.init(__merge(__def.size, __size));
    } else {
        __def.init(__size);
    }
    __def.type++;
    updated = true;
}


void KnowledgePropagatior::visitJump(jump_stmt *) {}
void KnowledgePropagatior::visitBranch(branch_stmt *) {}
void KnowledgePropagatior::visitCall(call_stmt *) {}
void KnowledgePropagatior::visitLoad(load_stmt *) {}
void KnowledgePropagatior::visitStore(store_stmt *) {}
void KnowledgePropagatior::visitReturn(return_stmt *) {}
void KnowledgePropagatior::visitGet(get_stmt *) {}
void KnowledgePropagatior::visitPhi(phi_stmt *ctx) {
    auto &__def = defMap[ctx->dest];
    if (__def.type == __def.UNCERTAIN) return;
    if (ctx->dest->type != typeinfo {int_type::ptr(),0})
        return void(__def.type = defInfo::UNCERTAIN);

    auto &__visitor = blockMap[ctx->get_ptr <block>()]; 

    auto __temp = ctx->get_ptr <block> ();

    bool __first = true;
    sizeInfo __size {};

    for (auto [__block, __value] : ctx->list)
        if (__visitor.has_visit_from(__block)) {
            if (__first) {
                __first = false;
                __size = traceSize(__value);
            } else {
                __size = __merge(__size, traceSize(__value));
            }
        }

    if (__def.type != __def.UNDEFINED) {
        if (__def.size <= __size) return;
        if (__size.upper > __def.size.upper)
            __size.upper = INT32_MAX;
        if (__size.lower < __def.size.lower)
            __size.lower = INT32_MIN;
        __def.init(__merge(__size, __def.size));
    } else {
        __def.init(__size);
    }
    __def.type++;
    updated = true;
}

void KnowledgePropagatior::visitUnreachable(unreachable_stmt *) {}

/**
 * @return Whether lhs is no stronger than rhs.
 * 1. lhs state is contained within rhs state in bits.
 * 2. lhs valid bits are contained within rhs valid bits.
 */
bool operator <= (bitsInfo __lhs, bitsInfo __rhs) {
    return (__lhs.valid & __rhs.valid) == __lhs.valid
        && (__lhs.valid & __lhs.state) == (__rhs.valid & __lhs.state);
}
/**
 * @return Whether lhs is no stronger than rhs.
 * lhs's range must be larger than that of rhs.
 */
bool operator <= (sizeInfo __lhs, sizeInfo __rhs) {
    return __lhs.lower <= __rhs.lower
        && __lhs.upper >= __rhs.upper;
}

bitsInfo KnowledgePropagatior::traceBits(definition *__def) {
    if (auto *__val = __def->as <integer_constant> ()) {
        return bitsInfo {__val->value};
    } else if (auto *__tmp = __def->as <temporary> ()) {
        auto &__info = defMap[__tmp];
        return __info.type == defInfo::UNCERTAIN ? bitsInfo {} : __info.bits;
    } else return bitsInfo {};
}

sizeInfo KnowledgePropagatior::traceSize(definition *__def) {
    if (auto *__val = __def->as <integer_constant> ()) {
        return sizeInfo {__val->value};
    } else if (auto *__tmp = __def->as <temporary> ()) {
        auto &__info = defMap[__tmp];
        return __info.type == defInfo::UNCERTAIN ? sizeInfo {} : __info.size;
    } else return sizeInfo {};
}

void knowledge::init(sizeInfo __size) {
    size = __size;
    if (size.lower == size.upper)
        bits = bitsInfo {size.lower};
    else
        bits = {};
}

void knowledge::init(bitsInfo __bits) {
    bits = __bits;
    if (bits.valid == -1)
        size = sizeInfo {bits.state};
    else
        size = {};
}



} // namespace dark::IR

