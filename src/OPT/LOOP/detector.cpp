#include "queue.h"
#include "LOOP/detector.h"
#include "dominant.h"
#include <ranges>
#include <algorithm>

namespace dark::IR {

/* Some internal method to initialize. */
static loopInfo *getLoop(block *__block) {
    if (!__block->loop) {
        __block->loop = new loopInfo;
        __block->loop->header = __block;
    }
    return __block->loop;
}

/**
 * @brief Build the loop nest tree.
 * Set the depth information for the loopInfo.
 */
static void buildTree(block *__block) {
    if (auto *__loop = __block->loop) {
        if (__loop->header == __block) {
            auto *__prev  = __loop->parent;
            __loop->depth = __prev ? __prev->depth + 1 : 1;
            __block->comments = std::format("loop depth: {}", __loop->depth);
        } else {
            using std::ranges::binary_search;
            while (__loop && !binary_search(__loop->body, __block))
                __loop = __loop->parent;
            __block->loop = __loop; // Set the real pointer.
            auto __depth = __loop ? __loop->depth : 0;
            __block->comments = std::format("loop depth: {}", __depth);
        }
    } else {
        __block->comments = "loop depth: 0";
    }
}

/**
 * @brief A backedge on dom tree. It's rather easy to implement.
 */
static void detectBackEdge(block *__block) {
    for (auto *__next : __block->next)
        if (std::ranges::find(__block->dom, __next) != __block->dom.end())
            getLoop(__next)->body.push_back(__block);
}

/**
 * @brief Use silly bfs to detect.
 */
static void detectLoopBody(block *__block) {
    if (!__block->loop) return;
    auto *__loop = __block->loop;
    work_queue <block *> worklist;
    worklist.container().assign(__loop->body.begin(), __loop->body.end());
    __loop->body.clear();
    while (!worklist.empty()) {
        auto *__front = worklist.pop_value();
        __loop->body.push_back(__front);
        if (__front == __block) continue;
        for (auto *__prev : __front->prev) worklist.push(__prev);
    }
}

/**
 * @brief Use dom tree to set up the information.
 */
static void detectLoopNest(block *__block) {
    auto *__idom = __block->idom;
    if (!__idom) return;
    auto *__prev = __idom->loop;
    if (!__block->loop) __block->loop = __prev;
    else { // Header block case.
        using std::ranges::binary_search;
        auto *__loop = __block->loop;
        std::ranges::sort(__loop->body);
        while (__prev && !binary_search(__prev->body, __block))
            __prev = __prev->parent;
        __loop->parent = __prev;
    }
}


LoopNestDetector::LoopNestDetector(function *__func) {
    if (!checkProperty(__func)) return;
    for (auto *__block : __func->rpo) detectBackEdge(__block);
    for (auto *__block : __func->rpo) detectLoopBody(__block);
    for (auto *__block : __func->rpo) detectLoopNest(__block);
    for (auto *__block : __func->rpo) buildTree(__block);
}


/* Return whether there's something cleaned. */
bool LoopNestDetector::clean(function *__func) {
    std::vector <loopInfo *> loop;
    for (auto *__block : __func->data) {
        if (auto *__loop = __block->loop) {
            if (__loop->header == __block)
                loop.push_back(__loop);
            __block->loop = nullptr;
        }
    }
    for (auto *__loop : loop) delete __loop;
    return !loop.empty();
}

void LoopNestDetector::setProperty(function *) {}
bool LoopNestDetector::checkProperty(function *__func) {
    if (__func->is_unreachable()) return false;
    runtime_assert(__func->has_cfg, "No CFG!");
    if (!(__func->has_dom && __func->has_rpo) || __func->is_post)
        dominantMaker { __func };
    if (clean(__func)) warning("Previous loop info not cleaned yet!");
    return true;
}



} // namespace dark::IR
