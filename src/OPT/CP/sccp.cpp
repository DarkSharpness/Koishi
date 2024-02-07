#include "CP/sccp.h"
#include "CP/calc.h"
#include "IRnode.h"
#include "dominant.h"
#include <algorithm>

namespace dark::IR {

static bool __may_go_constant(statement *__stmt) {
    return __stmt->as <binary_stmt> ()
        || __stmt->as <compare_stmt> ()
        || __stmt->as <call_stmt> ()
        || __stmt->as <phi_stmt> ();
}

static definition *__merge_defs(definition *__lhs, definition *__rhs) {
    if (__lhs->as <undefined> ()) return __rhs;
    if (__rhs->as <undefined> ()) return __lhs;
    return __lhs == __rhs ? __lhs : nullptr;
}

definition *ConstantPropagatior::getValue(definition *__def) {
    auto *__tmp = __def->as <temporary> ();
    if (!__tmp) return __def;
    /**
     * If mapping to non-null, it is constant or unknown value.
     * Otherwise, this variable is non-deterministic.
     * Even so, the variable can still be itself in the worst case.
    */
    return defMap[__tmp].value ? : __def;
}

ConstantPropagatior::ConstantPropagatior(function *__func, bool __hasDom)
: hasDomTree(__hasDom) {
    if (__func->is_unreachable()) return;

    for (auto __block : __func->data) initInfo(__block);
    CFGworklist.push({.from = nullptr,.to = __func->data[0]});
    do {
        while (!CFGworklist.empty()) {
            auto __front = CFGworklist.front();
            CFGworklist.pop();
            updateCFG(__front);
        }
        while (!SSAworklist.empty()) {
            auto __front = SSAworklist.front();
            SSAworklist.pop();
            updateSSA(__front);
        }
    } while (!CFGworklist.empty());
    for (auto __block : __func->data) modifyValue(__block);
}

void ConstantPropagatior::initInfo(block *__block) {
    visitBlock(__block, [__block](statement *__stmt) { __stmt->set_ptr(__block); });

    auto &&__set_undefined = [this](temporary *__tmp) -> void {
        defMap[__tmp].value = IRpool::create_undefined(__tmp->type);
    };
    auto &&__set_non_const = [this](temporary *__tmp) -> void {
        defMap[__tmp].value = nullptr;
    };
    auto &&__operation = [=, this](statement *__stmt) -> void {
        if (auto __def = __stmt->get_def()) {
            if (__may_go_constant(__stmt))  __set_undefined(__def);
            else /* No need to set const */ __set_non_const(__def);
        }
        for (auto __use : __stmt->get_use())
            if (auto __tmp = __use->as <temporary> ())
                defMap[__tmp].useList.push_back(__stmt);
    };

    visitBlock(__block, __operation);
}

void ConstantPropagatior::updateCFG(CFGinfo __info) {
    auto [__from, __to] = __info; // Only visit an edge once.
    auto &__visitor = blockMap[__to];
    if (!__visitor.try_visit(__from)) return;

    for (auto __stmt : __to->phi) visitPhi(__stmt);

    if (__visitor.is_first_visit())
        for (auto __stmt : __to->data) visitStmt(__stmt);

    if (auto __jump = __to->flow->as <jump_stmt> ())
        CFGworklist.push({__to, __jump->dest});
    else if (auto __br = __to->flow->as <branch_stmt> ())
        visitBranch(__br);
}

void ConstantPropagatior::updateSSA(SSAinfo __info) {
    if (auto * __phi = __info->as <phi_stmt> ())
        return visitPhi(__phi);
    else {
        auto &__visitor = blockMap[__info->get_ptr <block>()]; 
        if (!__visitor.is_not_visited())
            visitStmt(__info);
    }
}

void ConstantPropagatior::visitPhi(phi_stmt *__stmt) {
    auto &__visitor = blockMap[__stmt->get_ptr <block>()]; 
    if (__visitor.is_not_visited()) return;

    valueList.clear();
    for (auto [__block , __value] : __stmt->list)
        if (__visitor.has_visit_from(__block))
            valueList.push_back(getValue(__value));
    tryUpdate(__stmt);
}

void ConstantPropagatior::visitStmt(statement *__stmt) {
    if (auto *__br = __stmt->as <branch_stmt> ())
        return visitBranch(__br);
    if (!__may_go_constant(__stmt)) return;

    valueList.clear();
    for (auto __use : __stmt->get_use())
        valueList.push_back(getValue(__use));
    tryUpdate(__stmt);
}

void ConstantPropagatior::tryUpdate(statement *__stmt) {
    auto *__new = ConstantCalculator {__stmt, valueList, hasDomTree}.result;
    auto *__def = __stmt->get_def();
    if (__new == __def) return;

    auto &__info = defMap[__def];
    __new = __merge_defs(__new, __info.value);
    if (__info.value == __new) return;

    /* Update and push to worklist. */
    __info.value = __new;
    for (auto __use : __info.useList) SSAworklist.push(__use);
}

void ConstantPropagatior::visitBranch(branch_stmt *__stmt) {
    auto __cond = getValue(__stmt->cond);
    if (__cond->as <undefined> ()) return;

    auto *__block = __stmt->get_ptr <block> ();
    if (auto __bool = __cond->as <boolean_constant> ()) {
        CFGworklist.push({.from = __block,.to = __stmt->branch[__bool->value] });
    } else {
        CFGworklist.push({.from = __block,.to = __stmt->branch[0] });
        CFGworklist.push({.from = __block,.to = __stmt->branch[1] });
    }
}

void ConstantPropagatior::modifyValue(block *__block) {
    if (hasDomTree) {
        auto &__dom = __block->dom;
        std::sort(__dom.begin(), __dom.end());
    }
    auto &&__operation = [this](statement *__node) -> void {
        for (auto __old : __node->get_use()) {
            auto __new = getValue(__old);
            if (hasDomTree) {
                /* Avoid invalid copy propogation. */
                if (auto __tmp = __new->as <temporary> ()) {
                    auto __from = __tmp->def->get_ptr <block> ();
                    auto __to   = __node->get_ptr <block> ();
                    auto &__dom = __to->dom;
                    if (!std::binary_search(__dom.begin(), __dom.end(), __from))
                        continue;
                }
            }
            if (__new && __old != __new) __node->update(__old,__new);
        }
    };
    visitBlock(__block, __operation);
}

} // namespace dark::IR
