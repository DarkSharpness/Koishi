#include "VN/gvn.h"
#include "IRnode.h"
#include "dominant.h"

namespace dark::IR {

GlobalValueNumberPass::GlobalValueNumberPass(function *__func) {
    if (!checkProperty(__func)) return;
    makeDomTree(__func);
    collectUse(__func);
    visitGVN(__func->data[0]);
    setProperty(__func);
}

void GlobalValueNumberPass::collectUse(function *__func) {
    auto &&__operation = [this](statement *__node) {
        for (auto __use : __node->get_use())
            if (auto __var = __use->as <non_literal> ())
                useCount[__var]++;
    };
    for (auto __block : __func->data) visitBlock(__block, __operation);
}



/**
 * @brief Use the space of frontier to store the
 * successor of the block on the dominance tree.
 */
void GlobalValueNumberPass::makeDomTree(function *__func) {
    for (auto __block : __func->rpo) {
        visitBlock(__block, [__block](statement *__stmt) { __stmt->set_ptr(__block); });
        __block->fro.clear();
        auto __idom = __block->idom;
        if (!__idom) continue;
        __idom->fro.push_back(__block);
    }
}

void GlobalValueNumberPass::visitGVN(block *__block) {
    clearMemoryInfo();
    for (auto __stmt : __block->phi) visitPhi(__stmt);
    for (auto __stmt : __block->data) visit(__stmt);
    visit(__block->flow);
    visited.insert(__block);
    auto __mmap = memMap;

    for (auto __next : __block->fro) visitGVN(__next);

    visited.erase(__block);
    return removeHash(__block);
}

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

definition *GlobalValueNumberPass::getValue(definition *__def)  {
    auto __tmp = __def->as <temporary> ();
    if (!__tmp) return __def;
    return defMap.try_emplace(__tmp, __def).first->second;
}

expression::expression(get_stmt *__get)
: type(GETADDR), op(__get->member), lhs(__get->addr), rhs(__get->index) {}

expression::expression(binary_stmt *__bin)
: type(BINARY), op(__bin->op), lhs(__bin->lval), rhs(__bin->rval) {}

expression::expression(compare_stmt *__cmp)
: type(COMPARE), op(__cmp->op), lhs(__cmp->lval), rhs(__cmp->rval) {}


} // namespace dark::IR
