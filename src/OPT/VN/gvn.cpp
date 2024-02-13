#include "VN/gvn.h"
#include "IRnode.h"
#include "dominant.h"
#include <ranges>

namespace dark::IR {

GlobalValueNumberPass::GlobalValueNumberPass(function *__func) {
    if (!checkProperty(__func)) return;
    makeDomTree(__func);
    collectUse(__func);
    visitGVN(__func->data[0]);
    removeDead(__func);
    setProperty(__func);
}

/**
 * @brief A rpo walk on the dominator tree.
 */
void GlobalValueNumberPass::visitGVN(block *__block) {
    clearMemoryInfo();
    for (auto __stmt : __block->phi) visitPhi(__stmt);
    for (auto __stmt : __block->data) visit(__stmt);
    visit(__block->flow);
    visited.insert(__block);
    for (auto __next : __block->fro) visitGVN(__next);
    visited.erase(__block);
    return removeHash(__block);
}

void GlobalValueNumberPass::visitPhi(phi_stmt *__phi) {
    temporary  *__def = __phi->dest;
    definition *__tmp = nullptr;
    bool    __failure = false; 
    for (auto [__block, __use] : __phi->list) {
        if (__use == __def) continue;
        if (__use->as <undefined> ()) continue;
        else if (__tmp == nullptr) __tmp = __use;
        else if (__tmp != __use) { __failure = true; break; }
    }
    if (__failure) return; // No need to remap, failed!
    if (!__tmp) defMap[__def] = IRpool::create_undefined(__def->type);
    else if (auto __val = __tmp->as <temporary> ()) {
        auto *__from = __val->def->get_ptr <block> ();
        if (!visited.count(__from)) return; // Def do not dom its use.
    }
    return updateValue(__def, defMap[__def] = __tmp);
}

void GlobalValueNumberPass::visitCall(call_stmt *ctx) {
    return (void)analyzeCall(ctx);
}

void GlobalValueNumberPass::visitLoad(load_stmt *ctx) {
    return updateValue(ctx->dest, defMap[ctx->dest] = analyzeLoad(ctx));
}

void GlobalValueNumberPass::visitStore(store_stmt *ctx) {
    return analyzeStore(ctx);
}

void GlobalValueNumberPass::visitGet(get_stmt *__get) {
    auto [__iter, __success] = exprMap.try_emplace(__get, __get->dest);
    return updateValue(__get->dest, defMap[__get->dest] = __iter->second);
}

void GlobalValueNumberPass::visitBinary(binary_stmt *ctx) {
    /**
     * This is intended to hold the old value.
     * We may not necessarily update the value of the expression
     * with the new value, since it may worsen the register
     * allocation pressure afterwards.
    */
    auto __lval = ctx->lval;
    auto __rval = ctx->rval;
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
        auto *__new = defMap[ctx->dest] = result;
        result = nullptr;
        return updateValue(ctx->dest, __new);
    } else { /* Tries to register a value, perform global value numerbing. */
        auto [__iter, __success] = exprMap.try_emplace(ctx, ctx->dest);
        defMap[ctx->dest] = __iter->second;
        return updateValue(ctx->dest, __iter->second);
    }
}

void GlobalValueNumberPass::visitCompare(compare_stmt *ctx) {
    updateCompare(ctx);
    if (result != nullptr) {
        auto *__new = defMap[ctx->dest] = result;
        result = nullptr;
        return updateValue(ctx->dest, __new);
    } else {
        auto [__iter, __success] = exprMap.try_emplace(ctx, ctx->dest);
        defMap[ctx->dest] = __iter->second;
        return updateValue(ctx->dest, __iter->second);
    }
}

/**
 * TODO: Spread the condition to the next block and 
 * its domSet.
 * Use a stack to maintain the condition.
 * like %x != 0, %y < 0.
 * This can be used to optimize the condition branch. 
 */
void GlobalValueNumberPass::visitBranch(branch_stmt *) {}
void GlobalValueNumberPass::visitJump(jump_stmt *) {}
void GlobalValueNumberPass::visitUnreachable(unreachable_stmt *) {}
void GlobalValueNumberPass::visitReturn(return_stmt *) {}

} // namespace dark::IR


/* Some other part. */
namespace dark::IR {

void GlobalValueNumberPass::removeHash(block *__block) {
    for (auto __stmt : __block->data) {
        if (auto __bin = __stmt->as <binary_stmt> ()) {
            if (defMap[__bin->dest] == __bin->dest)
                exprMap.erase(__bin);
        } else if (auto __cmp = __stmt->as <compare_stmt> ()) {
            if (defMap[__cmp->dest] == __cmp->dest)
                exprMap.erase(__cmp);
        } else if (auto __get = __stmt->as <get_stmt> ()) {
            if (defMap[__get->dest] == __get->dest)
                exprMap.erase(__get);
        }
    }
}

void GlobalValueNumberPass::makeDomTree(function *__func) {
    for (auto __block : __func->rpo) {
        visitBlock(__block, [__block](statement *__stmt) { __stmt->set_ptr(__block); });
        __block->fro.clear();
        auto __idom = __block->idom;
        if (!__idom) continue;
        __idom->fro.push_back(__block);
    }
}

/**
 * @brief Update all use with the latest value.
 */
void GlobalValueNumberPass::updateValue(temporary *__old, definition *__new) {
    if (__old == __new) return;
    for (auto &__vec : useMap | std::views::values)
        for (auto *__use  : __vec)
            __use->update(__old, __new);
}

void GlobalValueNumberPass::removeDead(function *__func) {
    auto &&__filter = std::views::filter([this](statement *__stmt) {
        auto __store = __stmt->as <store_stmt> ();
        if (!__store) return true;
        return deadStore.count(__store) == 0;
    });

    for (auto __block : __func->rpo) {
        auto &&__range = __block->data | __filter;
        auto   __first = __block->data.begin();
        __block->data.resize(
            std::ranges::copy(__range, __first).out - __first);
    }
}


expression::expression(get_stmt *__get)
: type(GETADDR), op(__get->member), lhs(__get->addr), rhs(__get->index) {}

expression::expression(binary_stmt *__bin)
: type(BINARY), op(__bin->op), lhs(__bin->lval), rhs(__bin->rval) {}

expression::expression(compare_stmt *__cmp)
: type(COMPARE), op(__cmp->op), lhs(__cmp->lval), rhs(__cmp->rval) {}

void GlobalValueNumberPass::setProperty(function *__func) {
    __func->has_fro = false;
}

bool GlobalValueNumberPass::checkProperty(function *__func) {
    if (__func->is_unreachable()) return false;
    runtime_assert(__func->has_cfg, "No CFG!");
    if (!(__func->has_dom && __func->has_rpo) || __func->is_post)
        dominantMaker { __func };
    return true;
}

} // namespace dark::IR
