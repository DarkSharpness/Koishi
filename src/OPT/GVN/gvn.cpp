#include "GVN/gvn.h"
#include "GVN/algebraic.h"
#include "IRnode.h"
#include "dominant.h"
#include <ranges>

/* Some other part. */
namespace dark::IR::__gvn {

/**
 * @brief Use the frontier field of block to store
 * the descendant of the block on the dominator tree.
 */
static void makeDomTree(function *__func) {
    for (auto __block : __func->rpo) {
        visitBlock(__block, [__block](statement *__stmt) { __stmt->set_ptr(__block); });
        __block->fro.clear();
        auto __idom = __block->idom;
        if (!__idom) continue;
        __idom->fro.push_back(__block);
    }
}

/* A rpo walk on the dominator tree. */
void GlobalValueNumberPass::visitGVN(block *__block) {
    memManager.reset();

    for (auto __stmt : __block->phi) visitPhi(__stmt);
    for (auto __stmt : __block->data) visit(__stmt);
    visit(__block->flow);

    visited.insert(__block);
    for (auto __next : __block->fro) visitGVN(__next);
    visited.erase(__block);

    return removeHash(__block);
}

void GlobalValueNumberPass::removeHash(block *__block) {
    auto &&__modify = [this](statement *__stmt) {
        auto __binary = __stmt->as <binary_stmt> ();
        if (!__binary) return;
        auto  __dst = __binary->dest;
        auto &__ref = this->defMap[__dst];
        if (__ref.is_const()) return;
        this->cseMap[__ref.get_expression()].remove(__dst);
    };
    visitBlock(__block, __modify);
}

/* Remove all dead stores. */
void GlobalValueNumberPass::removeDead(function *__func) {
    auto &&__filter = std::views::filter([this](statement *__stmt) {
        auto __store = __stmt->as <store_stmt> ();
        if (!__store) return true;
        return memManager.deadStore.count(__store) == 0;
    });

    for (auto __block : __func->rpo) {
        auto &&__range = __block->data | __filter;
        auto   __first = __block->data.begin();
        __block->data.resize(std::ranges::copy(__range, __first).out - __first);
    }
}

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

GlobalValueNumberPass::GlobalValueNumberPass(function *__func) {
    if (!checkProperty(__func)) return;
    makeDomTree(__func);
    visitGVN(__func->data[0]);
    removeDead(__func);
    setProperty(__func);
}

} // namespace dark::IR::__gvn
