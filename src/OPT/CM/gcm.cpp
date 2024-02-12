#include "IRnode.h"
#include "CM/gcm.h"
#include "LOOP/loop.h"
#include <ranges>

namespace dark::IR {


static bool __is_fixed(statement *__node) {
    return __node->as <call_stmt> ()
        || __node->as <memory_statement> ()
        || __node->as <phi_stmt> ()
        || __node->as <flow_statement> ();
}

static std::size_t __get_depth(block *__node) {
    return __node->get_val <std::size_t> ();
}

static void __set_depth(block *__node, std::size_t __depth) {
    __node->set_val <std::size_t> (__depth);
}

static block *__get_early(statement *__node) {
    return __node->get_ptr <block> ();
}

static std::size_t __get_loop_depth(block *__node) {
    return __node->loop ? __node->loop->depth : 0;
}

static void __set_early(statement *__node, block *__block) {
    __node->set_ptr(__block);
}

static block *__find_lca(block *__a, block *__b) {
    if (!__a) return __b;
    if (__get_depth(__a) < __get_depth(__b))
        std::swap(__a, __b);
    while (__get_depth(__a) > __get_depth(__b))
        __a = __a->idom;
    while (__a != __b) __a = __a->idom, __b = __b->idom;
    return __a;
}


GlobalCodeMotionPass::GlobalCodeMotionPass(function *__func) {
    if (!checkProperty(__func)) return;
    __entry = __func->data[0];

    for (auto __block : __func->rpo) initBlock(__block);

    auto &&__early = [this](statement *__node) { scheduleEarly(__node); };
    for (auto __block : __func->rpo) visitBlock(__block, __early);

    visited.clear();
    for (auto __block : __func->rpo) initUseList(__block);
    auto &&__later = [this](statement *__node) { scheduleLate(__node); };
    for (auto __block : __func->rpo) visitBlock(__block, __later);

    visited.clear();
    for (auto __block : __func->rpo) prepareMotion(__block);
    for (auto __block : __func->rpo) arrangeMotion(__block);

    setProperty(__func);
}

void GlobalCodeMotionPass::initBlock(block *__block) {
    const std::size_t __depth = __block->idom ?
        __get_depth(__block->idom) + 1 : 0;
    __set_depth(__block, __depth);

    for (auto __stmt : __block->phi)
        __set_early(__stmt, __block);

    for (auto __stmt : __block->data)
        if (__is_fixed(__stmt))
            __set_early(__stmt, __block);
        else
            __set_early(__stmt, __entry);

    __set_early(__block->flow, __block);
}

void GlobalCodeMotionPass::scheduleEarly(statement *__node) {
    if (!visited.insert(__node).second) return;
    block *__block = __entry;
    for (auto __use : __node->get_use()) {
        auto __tmp = __use->as <temporary> ();
        if (!__tmp) continue;
        scheduleEarly(__tmp->def);
        auto __prev = __get_early(__tmp->def);
        if (__get_depth(__prev) > __get_depth(__block))
            __block = __prev;
    }
    if (__is_fixed(__node)) return;
    __set_early(__node, __block);
}

void GlobalCodeMotionPass::initUseList(block *__block) {
    auto &&__operation = [this, __block](statement *__stmt) -> void {
        for (auto __use : __stmt->get_use()) {
            auto __tmp = __use->as <temporary> ();
            if (!__tmp) continue;
            defMap[__tmp->def].useList.push_back(__stmt);
        }
        if (__is_fixed(__stmt))
            defMap[__stmt].attached = __block;
    };
    visitBlock(__block, __operation);
}

void GlobalCodeMotionPass::scheduleLate(statement *__node) {
    if (!visited.insert(__node).second) return;
    if (__is_fixed(__node)) return;
    block *__lca {};
    for (auto __use : defMap[__node].useList) {
        scheduleLate(__use);
        if (auto *__phi = __use->as <phi_stmt> ()) {
            auto *__def = __node->get_def();
            for (auto [__prev, __value] : __phi->list) {
                if (__value == __node->get_def())
                    __lca = __find_lca(__lca, __prev);
            }
        } else { __lca = __find_lca(__lca, defMap[__use].attached); }
    }
    return scheduleBlock(__node, __get_early(__node), __lca);
}


void GlobalCodeMotionPass::scheduleBlock
    (statement *__node, block *__first, block *__last) {
    block *__best = __last;
    do {
        if (__get_loop_depth(__last) < __get_loop_depth(__best))
            __best = __last;
        if (__last == __first) break;
        __last = __last->idom;
    } while (true);
    defMap[__node].attached = __best;
}


void GlobalCodeMotionPass::prepareMotion(block *__block) {
    for (auto __stmt : __block->phi)
        __stmt->set_ptr(__block);

    for (auto __stmt : __block->data) {
        auto &__def = defMap[__stmt];
        newList[__def.attached].push_back(__stmt);
        __stmt->set_ptr(__def.attached);
    }

    __block->flow->set_ptr(__block);
}

void GlobalCodeMotionPass::arrangeMotion(block *__block) {
    auto &__list = newList[__block];

    __block->data.clear();
    for (auto __stmt : __list)
        if (__is_fixed(__stmt))
            motionDfs(__stmt, __block);

    for (auto __stmt : __list)
        motionDfs(__stmt, __block);

}


void GlobalCodeMotionPass::motionDfs(statement *__node, block *__block) {
    if (!visited.insert(__node).second) return;

    for (auto __use : __node->get_use()) {
        auto __tmp = __use->as <temporary> ();
        if (!__tmp) continue;

        auto __def = __tmp->def;
        if (__def->as <phi_stmt> ()) continue;
        auto __prev = __tmp->def->get_ptr();
        if (__prev != __block) continue;

        motionDfs(__def, __block);
    }

    __block->data.push_back(__node);
}



bool GlobalCodeMotionPass::checkProperty(function *__func) {
    if (__func->is_unreachable()) return false;
    runtime_assert(__func->has_dom, "GCM requires dominator tree to be built");
    return true;
}

void GlobalCodeMotionPass::setProperty(function *) {}


} // namespace dark::IR
