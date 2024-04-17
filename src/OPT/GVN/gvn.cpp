#include "GVN/gvn.h"
#include "GVN/algebraic.h"
#include "IRnode.h"
#include "dominant.h"
#include <ranges>

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

GlobalValueNumberPass::GlobalValueNumberPass(function *__func) {
    if (!checkProperty(__func)) return;
    makeDomTree(__func);
    visitGVN(__func->data[0]);
    removeDead(__func);
    setProperty(__func);
}

/** @brief A rpo walk on the dominator tree. */
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

void GlobalValueNumberPass::visitPhi(phi_stmt *__phi) {
    runtime_assert(false, "TODO");
    // temporary  *__def = __phi->dest;
    // definition *__tmp = nullptr;
    // bool    __failure = false; 
    // for (auto [__block, __use] : __phi->list) {
    //     if (__use == __def) continue;
    //     if (__use->as <undefined> ()) continue;
    //     else if (__tmp == nullptr) __tmp = __use;
    //     else if (__tmp != __use) { __failure = true; break; }
    // }
    // if (__failure) return; // No need to remap, failed!
    // if (!__tmp) defMap[__def] = IRpool::create_undefined(__def->type);
    // else if (auto __val = __tmp->as <temporary> ()) {
    //     auto *__from = __val->def->get_ptr <block> ();
    //     if (!visited.count(__from)) return; // Def do not dom its use.
    // }
    // return updateValue(__def, defMap[__def] = __tmp);
}

void GlobalValueNumberPass::visitCall(call_stmt *ctx) {
    return (void)memManager.analyzeCall(ctx);
}

void GlobalValueNumberPass::visitLoad(load_stmt *ctx) {
    return (void)memManager.analyzeLoad(ctx);
}

void GlobalValueNumberPass::visitStore(store_stmt *ctx) {
    return (void)memManager.analyzeStore(ctx);
}

void GlobalValueNumberPass::visitGet(get_stmt *__get) {
    runtime_assert(false, "TODO");
    // auto [__iter, __success] = exprMap.try_emplace(__get, __get->dest);
    // return updateValue(__get->dest, defMap[__get->dest] = __iter->second);
}

void GlobalValueNumberPass::visitBinary(binary_stmt *ctx) {
    auto __algebra = algebraicSimplifier { this->data };
    __algebra.visit(ctx->op, getNumber(ctx->lval), getNumber(ctx->rval));
    auto __result = std::move(__algebra.result);
    if (auto *__number = std::get_if <number_t> (&__result)) {
        /* Reduced to a existing number. */
        defMap[ctx->dest] = *__number;
    } else {
        auto __expression = std::get <expression> (__result);
        auto [__iter, __success] = numMap.try_emplace(__expression); 
        if (__success) { // A new expression.
            __iter->second = data.size();
            data.push_back(__expression);
        }
        defMap[ctx->dest] = { __iter->second, type_t::BINARY };
    }
}

void GlobalValueNumberPass::visitCompare(compare_stmt *ctx) {
    runtime_assert(false, "TODO");
}

number_t GlobalValueNumberPass::getNumber(definition *__def) {
    if (auto __integer = __def->as <integer_constant> ())
        return __integer->value;
    if (auto __boolean = __def->as <boolean_constant> ())
        return __boolean->value;
    auto [__iter, __success] = defMap.try_emplace(__def);
    if (__success) {
        __iter->second = { (int)data.size(), type_t::UNKNOWN };
        data.emplace_back(++unknown_count);
    }
    return __iter->second;
}

/* Mapping from a value index to number. */
// number_t GlobalValueNumberPass::getNumber(int __index) {
//     return number_t { __index, data[__index].get_type() };
// }

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
    runtime_assert(false, "TODO");
}

/* Update all use with the latest value. */
void GlobalValueNumberPass::updateValue(temporary *__old, definition *__new) {
    runtime_assert(false, "TODO");
}

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

} // namespace dark::IR::__gvn
