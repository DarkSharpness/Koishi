#include "GVN/gvn.h"
#include "GVN/algebraic.h"
#include "IRnode.h"
#include "dominant.h"

namespace dark::IR::__gvn {


void equal_set::insert(definition *__d) {
    if (this->best == nullptr) this->best = __d;
}

void equal_set::remove(definition *__d) {
    if (this->best == __d) this->best = nullptr;
}

definition *equal_set::top() { return this->best; }

/* Update all use with the latest value. */
void GlobalValueNumberPass::updateValue(temporary *__old, definition *__new) {
    defMap[__old] = getNumber(__new);
}

definition *GlobalValueNumberPass::pickBest(number_t __n, typeinfo __tp) {
    if (__n.is_const()) {
        if (__is_int_type(__tp))
            return IRpool::create_integer(__n.get_const());
        else
            return IRpool::create_boolean(__n.get_const());
    } else {
        return cseMap[__n.get_expression()].top();
    }
}

number_t GlobalValueNumberPass::getNumber(definition *__def) {
    if (auto __integer = __def->as <integer_constant> ())
        return __integer->value;
    if (auto __boolean = __def->as <boolean_constant> ())
        return __boolean->value;
    auto [__iter, __success] = defMap.try_emplace(__def);
    if (__success) {
        auto __expression = &unknown.emplace_back(unknown.size());
        __iter->second = number_t { __expression, type_t::UNKNOWN };
        cseMap[*__expression].insert(__def);
    }
    return __iter->second;
}

definition *GlobalValueNumberPass::pickBest(definition *__def) {
    /* For constants and unknowns, the best is itself. */
    auto __iter = defMap.find(__def);
    if (__iter == defMap.end()) return __def;
    return pickBest(__iter->second, __def->get_value_type());
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
    for (auto &__arg : ctx->args) __arg = pickBest(__arg);
    memManager.analyzeCall(ctx);

    // If pure, we may still number the result.
}

void GlobalValueNumberPass::visitLoad(load_stmt *ctx) {
    ctx->addr = pickBest(ctx->addr);
    auto *__loaded = memManager.analyzeLoad(ctx);
    if (ctx->dest != __loaded)
        return updateValue(ctx->dest, __loaded);
}

void GlobalValueNumberPass::visitStore(store_stmt *ctx) {
    ctx->addr = pickBest(ctx->addr);
    ctx->src_ = safe_cast <non_literal *> (pickBest(ctx->src_));
    return (void)memManager.analyzeStore(ctx);
}

void GlobalValueNumberPass::visitGet(get_stmt *__get) {
    runtime_assert(false, "TODO");
    // auto [__iter, __success] = exprMap.try_emplace(__get, __get->dest);
    // return updateValue(__get->dest, defMap[__get->dest] = __iter->second);
}

void GlobalValueNumberPass::visitBinary(binary_stmt *ctx) {
    runtime_assert(__is_int_type(ctx->dest->type), "Fail to handle");
    auto __algebra = algebraicSimplifier {};
    __algebra.visit(ctx->op, getNumber(ctx->lval), getNumber(ctx->rval));
    auto __result = std::move(__algebra.result);
    if (const auto *__number = std::get_if <number_t> (&__result)) {
        /* Reduced to a existing number, ctx is dead. */
        defMap[ctx->dest] = *__number;
    } else { /* Equal to an expression. */
        const auto __expression = std::get <expression> (__result);
        const auto __iter       = cseMap.try_emplace(__expression).first;
        auto &[__expr, __equal] = *__iter; // [ Expression ; EqualSet ]

        /* Update the equal set. */
        __equal.insert(ctx->dest);

        /* Update the defMap. */
        defMap[ctx->dest] = number_t { &__expr, type_t::BINARY };

        /* Update the context. */
        using _Op_t = binary_stmt::binary_op;
        ctx->op     = static_cast <_Op_t> (__expression.get_op());
        ctx->lval   = pickBest(__expression.get_l(), ctx->dest->type);
        ctx->rval   = pickBest(__expression.get_r(), ctx->dest->type);
    }
}

void GlobalValueNumberPass::visitCompare(compare_stmt *ctx) {
    ctx->lval = pickBest(ctx->lval);
    ctx->rval = pickBest(ctx->rval);
}

/**
 * TODO: Spread the condition to the next block and 
 * its domSet.
 * Use a stack to maintain the condition.
 * like %x != 0, %y < 0.
 * This can be used to optimize the condition branch. 
 */
void GlobalValueNumberPass::visitBranch(branch_stmt *ctx) {
    ctx->cond = pickBest(ctx->cond); 
}

void GlobalValueNumberPass::visitReturn(return_stmt *ctx) {
    if (ctx->retval) ctx->retval = pickBest(ctx->retval);
}

void GlobalValueNumberPass::visitJump(jump_stmt *) {}
void GlobalValueNumberPass::visitUnreachable(unreachable_stmt *) {}

} // namespace dark::IR::__gvn

