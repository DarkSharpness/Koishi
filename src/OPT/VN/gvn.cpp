#include "VN/gvn.h"
#include "IRnode.h"
#include "dominant.h"

namespace dark::IR {

GlobalValueNumberPass::GlobalValueNumberPass(function *__func) {
    if (__func->is_unreachable()) return;
    dominantMaker __dom { __func };    
    rpo = std::move(__dom.rpo);
    makeDomTree();
    visitGVN(__func->data[0]);
}

/**
 * @brief Use the space of frontier to store the
 * successor of the block on the dominance tree.
 */
void GlobalValueNumberPass::makeDomTree() {
    for (auto __block : rpo) {
        visitBlock(__block, [__block](statement *__stmt) { __stmt->set_ptr(__block); });
        __block->fro.clear();
        auto __idom = __block->idom;
        if (!__idom) continue;
        __idom->fro.push_back(__block);
    }
}

void GlobalValueNumberPass::visitGVN(block *__block) {
    for (auto __stmt : __block->phi)    visitPhi(__stmt);
    for (auto __stmt : __block->data)   visit(__stmt);
    for (auto __next : __block->next)   updateNext(__block, __next);
    visited.insert(__block);
    for (auto __next : __block->fro)    visitGVN(__next);
    visited.erase(__block);
    return removeHash(__block);
}

/* Merge the joint value of phi node. */
void GlobalValueNumberPass::visitPhi(phi_stmt *__phi) {
    temporary  *__def = __phi->dest;
    definition *__tmp = nullptr;
    bool    __failure = false; 
    for (auto [__block, __value] : __phi->list) {
        auto __use = getValue(__value);
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
    defMap[__def] = __tmp;
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
