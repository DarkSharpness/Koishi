#include "IRnode.h"
#include "DCE/adce.h"
#include "dominant.h"
#include "DCE/unreachable.h"
#include <ranges>

namespace dark::IR {

/* Return where should jumps to __block be changed to. */
static block *getFail(block *__block) { return __block->get_ptr <block> (); }
static block *setFail(block *__block, block *__fail) {
    return __block->set_ptr(__fail), __fail;
}


/**
 * @brief Build the fail jump tree and compress it.
 */
struct FailBuilder {
    using _Set_t = std::unordered_set <block *>;
    const _Set_t &  liveSet;
    _Set_t          visited;
    /* Use path compression to speed up the process. */
    block *build(block *__node) {
        if (!visited.insert(__node).second) return getFail(__node);
        auto *__idom = __node->idom;
        auto *__fail = (liveSet.contains(__node) || !__idom) ? __node : build(__idom);
        return setFail(__node, __fail);
    }
};

/**
 * @brief Make link for each statement in block.
 * This is used to help find the corresponding block fast.
 */
static void linkBlock(block *__block) {
    visitBlock(__block, [__block](statement *__stmt) { __stmt->set_ptr(__block); });
}

AggressiveElimination::AggressiveElimination(function *__func) {
    if (!checkProperty(__func)) return;
    dominantMaker __dom { __func , true };

    for (auto *__node : __func->data) linkBlock(__node);
    for (auto *__node : __func->data) markEffect(__node);
    spreadEffect();

    FailBuilder __fail { blockList.set() , {} };
    for (auto *__node : __func->data) __fail.build(__node);
    for (auto *__node : __func->data) removeDead(__node);

    __dom.clean(__func);
    unreachableRemover {__func};
    setProperty(__func);
}

void AggressiveElimination::markEffect(block *__blk) {
    auto &&__collect_effective = [this](statement *__stmt) -> void { 
        if (auto *__call = __stmt->as <call_stmt> ()) {
            if (__call->func->is_side_effective()) {
                stmtList.push(__stmt);
            }
        } else if (__stmt->as <store_stmt> ()
                || __stmt->as <return_stmt> ()) {
            stmtList.push(__stmt);
        }
    };
    visitBlock(__blk, __collect_effective);
}

void AggressiveElimination::spreadEffect() {
    auto &&__spread_stmt = [this](statement *__stmt) -> void {
        auto *__block = __stmt->get_ptr <block> ();
        if (auto *__phi = __stmt->as <phi_stmt> ())
            for (auto __prev : __block->prev)
                blockList.push(__prev);

        blockList.push(__block);
        for (auto __use : __stmt->get_use())
            if (auto __tmp = __use->as <temporary> ())
                stmtList.push(__tmp->def);
    };
    auto &&__spread_block = [this](block *__node) -> void {
        for (auto __prev : __node->fro) {
            auto *__br = __prev->flow->as <branch_stmt> ();
            runtime_assert(__br, "Invalid block flow.");
            stmtList.push(__br);         
        }
    };

    do {
        while (!stmtList.empty())
            __spread_stmt(stmtList.pop_value());
        while (!blockList.empty())
            __spread_block(blockList.pop_value());
    } while (!stmtList.empty());
}

void AggressiveElimination::removeDead(block *__block) {
    if (auto *__jump = __block->flow->as <jump_stmt> ()) {
        __jump->dest = getFail(__jump->dest);
    } else if (auto *__br = __block->flow->as <branch_stmt> ()) {
        __br->branch[0] = getFail(__br->branch[0]);
        __br->branch[1] = getFail(__br->branch[1]);
        if (__br->branch[0] == __br->branch[1])
            __block->flow = IRpool::allocate <jump_stmt> (__br->branch[0]);
    }

    auto &&__leave_effective = std::views::filter(
        [this](statement *__stmt) -> bool { return stmtList.contains(__stmt); });
    updateBlock(__block, __leave_effective);
}

void AggressiveElimination::setProperty(function *) {}

bool AggressiveElimination::checkProperty(function *__func) {
    return !__func->is_unreachable();
}




} // namespace dark::IR
