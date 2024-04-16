#include "CP/sckp.h"
#include "IRnode.h"
#include "dominant.h"
#include <ranges>
#include <algorithm>

namespace dark::IR {

static bool __may_go_constant(statement *__stmt) {
    return __stmt->as <binary_stmt> ()
        || __stmt->as <compare_stmt> ()
        // || __stmt->as <call_stmt> ()
        || __stmt->as <phi_stmt> ();
}

static definition *__merge_defs(definition *__lhs, definition *__rhs) {
    if (__lhs->as <undefined> ()) return __rhs;
    if (__rhs->as <undefined> ()) return __lhs;
    return __lhs == __rhs ? __lhs : nullptr;
}

/**
 * @brief A class which can simply analyze the size of scalar type.
 * It can also perform some easy strength reduction.
 */
KnowledgePropagatior::KnowledgePropagatior
    (function *__func, bool strengthReduced) : strengthReduced(strengthReduced) {
    if (!checkProperty(__func)) return;

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

    setProperty(__func);
}

void KnowledgePropagatior::initInfo(block *__block) {
    visitBlock(__block, [__block](statement *__stmt) { __stmt->set_ptr(__block); });

    auto &&__set_undefined = [this](temporary *__tmp) -> void {
        defMap[__tmp].type = defInfo::UNDEFINED;
    };
    auto &&__set_non_const = [this](temporary *__tmp) -> void {
        defMap[__tmp].type = defInfo::UNCERTAIN;
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

void KnowledgePropagatior::updateCFG(CFGinfo __info) {
    auto [__from, __to] = __info; // Only visit an edge once.
    auto &__visitor = blockMap[__to];
    if (!__visitor.try_visit(__from)) return;

    for (auto __stmt : __to->phi) updatePhi(__stmt);

    if (__visitor.is_first_visit())
        for (auto __stmt : __to->data) updateStmt(__stmt);

    if (auto __jump = __to->flow->as <jump_stmt> ())
        CFGworklist.push({__to, __jump->dest});
    else if (auto __br = __to->flow->as <branch_stmt> ())
        updateBranch(__br);
}

void KnowledgePropagatior::updateSSA(SSAinfo __info) {
    if (auto * __phi = __info->as <phi_stmt> ())
        return updatePhi(__phi);
    else {
        auto &__visitor = blockMap[__info->get_ptr <block>()]; 
        if (!__visitor.is_not_visited())
            updateStmt(__info);
    }
}

void KnowledgePropagatior::updatePhi(phi_stmt *__stmt) {
    auto &__visitor = blockMap[__stmt->get_ptr <block>()]; 
    if (__visitor.is_not_visited()) return;
    tryUpdate(__stmt);
}

void KnowledgePropagatior::updateStmt(statement *__stmt) {
    if (auto *__br = __stmt->as <branch_stmt> ())
        return updateBranch(__br);
    if (!__may_go_constant(__stmt)) return;
    tryUpdate(__stmt);
}

void KnowledgePropagatior::tryUpdate(statement *__stmt) {
    visit(__stmt);
    if (!updated) return;
    updated = false;
    auto __tmp = __stmt->get_def();
    for (auto __use : defMap[__tmp].useList) SSAworklist.push(__use);
}

void KnowledgePropagatior::updateBranch(branch_stmt *__stmt) {
    auto *__block = __stmt->get_ptr <block> ();
    auto *__tmp = __stmt->cond->as <temporary> ();
    if (!__tmp) {
        CFGworklist.push({.from = __block,.to = __stmt->branch[0] });
        CFGworklist.push({.from = __block,.to = __stmt->branch[1] });
        return;
    }
    auto &__cond = defMap[__tmp];
    if (__cond.type == __cond.UNDEFINED) return;
    if (__cond.is_const()) {
        CFGworklist.push({.from = __block,.to = __stmt->branch[__cond.value()] });
    } else {
        CFGworklist.push({.from = __block,.to = __stmt->branch[0] });
        CFGworklist.push({.from = __block,.to = __stmt->branch[1] });
    }
}

void KnowledgePropagatior::modifyValue(block *__block) {
    auto &&__operation = [this](statement *__node) -> void {
        for (auto __old : __node->get_use()) {
            auto __tmp = __old->as <temporary> ();
            if (!__tmp) continue;
            auto &__info = defMap[__tmp];
            if (__info.is_undefined()) {
                __node->update(__old, IRpool::create_undefined(__tmp->type));
            } else if (__info.is_const()) {
                if (__tmp->type == typeinfo{ bool_type::ptr() , 0 })
                    __node->update(__old, IRpool::create_boolean(__info.value()));
                else
                    __node->update(__old, IRpool::create_integer(__info.value()));
            } else if (strengthReduced && !__info.is_uncertain()) {
                if (auto __bin = __node->as <binary_stmt> ()) {
                    auto __rhs = __bin->rval->as <integer_constant> ();
                    if (!__rhs || __info.size.lower < 0) continue;
                    using _Tp = decltype(__bin->op);
                    using enum _Tp;
                    // Division by power of 2
                    if (__bin->op == DIV && std::has_single_bit<unsigned>(__rhs->value)) {
                        __bin->op = SHR;
                        auto __log = std::countr_zero<unsigned>(__rhs->value);
                        __bin->rval = IRpool::create_integer(__log);
                    } else if (__bin->op == MOD && std::has_single_bit<unsigned>(__rhs->value)) {
                        __bin->op = AND;
                        auto __msk = __rhs->value - 1;
                        __bin->rval = IRpool::create_integer(__msk);
                    }
                }
            }
        }
    };
    visitBlock(__block, __operation);
}

void KnowledgePropagatior::setProperty(function *) {}

bool KnowledgePropagatior::checkProperty(function *__func) {
    return !__func->is_unreachable();
}

} // namespace dark::IR
