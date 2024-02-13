#include "VN/gvn.h"
#include "IRnode.h"
#include <algorithm>

namespace dark::IR {

enum class addressType {
    GLOBAL,     // Global variable  (always unique)
    GETMEMBER,  // Access member (field access, watch base pointer)
    PHI,        // Phi result       (conflict almost all)
    GETARRAY,  // Access array  (field access, watch base, index)
    LOCAL,  // Local variable   (almost unique)
    LOAD,   // Loaded out       (hard to trace, hardly analyze)
    NEW,    // New expression   (almost unique)
    CALL,   // Result of call   (hard to trace, may use attributes)
    ARGV,   // Function argument(hard to trace, but can't be a get)
};

static std::unordered_map <non_literal *, addressType> addrMap {};

static addressType __traceAddr(non_literal *__var) {
    auto [__iter, __success] = addrMap.try_emplace(__var);
    auto &__ans = __iter->second;
    if (!__success) return __ans;

    using enum addressType;
    if (__var->as <variable> ()) {
        if (__var->as <global_variable> ())
            return __ans = GLOBAL;
        if (__var->as <argument> ())
            return __ans = ARGV;
        else // Local variable, rare case.
            return __ans = LOCAL;
    }

    auto __tmp = __var->as <temporary> ();
    if (!__tmp) return __ans = PHI; // Assume the worst.
    auto __def = __tmp->def;
    if (auto __call = __def->as <call_stmt>()) {
        auto __func = __call->func;
        auto __dist = __func - IRpool::builtin_function;
        if (14 <= __dist && __dist <= 16)
            return __ans = NEW;
        else
            return __ans = CALL;
    } else if (__def->as <load_stmt> ()) {
        return __ans = LOAD;
    } else if (auto __get = __def->as <get_stmt> ()) {
        if (__get->member != __get->NPOS)
            return __ans = GETMEMBER;
        else // Array access.
            return __ans = GETARRAY;
    } else {
        return __ans = PHI; // Assume the worst.
    }
}

static bool __analyzeMember(non_literal *__lhs, non_literal *__rhs) {
    auto *__lget = __lhs->as <temporary> ()->def->as <get_stmt> ();
    auto *__rget = __rhs->as <temporary> ()->def->as <get_stmt> ();
    if (__lget->member != __rget->member) return false;
    return GlobalValueNumberPass::mayAliasing(__lget->addr, __rget->addr);
}

bool GlobalValueNumberPass::mayAliasing(definition *__l, definition *__r) {
    if (__l == __r) return true;
    auto __lhs = __l->as <non_literal> ();
    auto __rhs = __r->as <non_literal> ();
    if (!__lhs || !__rhs) { warning("WTF???"); return true; }
    if (__lhs->type != __rhs->type) return false;

    using enum addressType;
    auto __ltype = __traceAddr(__lhs);
    auto __rtype = __traceAddr(__rhs);
    auto [__min, __max] = std::minmax(__ltype, __rtype);
    switch (__min) {
        case GLOBAL:    return false;   // Global can't be aliasing.
        case GETMEMBER: return __min == __max ? __analyzeMember(__lhs, __rhs) : false;
        case PHI:       return true;    // Phi may be aliasing.
        case GETARRAY:  return __min == __max ? true : false;
        case LOCAL:     return false;   // Local can't be load/malloc/call return/argv.
        case LOAD:      return true;    // Load may be malloc/call/argv.
        case NEW:       return false;   // New can't be call/argv
        case CALL:      return true;    // Call may be argv.
        case ARGV:      return true;    // Argvs may be equal.
        default: __builtin_unreachable();
    }
}


void GlobalValueNumberPass::clearMemoryInfo() { memMap.clear(); }

void GlobalValueNumberPass::visitCall(call_stmt *__call) {
    for (auto __use : __call->args) {
        auto __val = getValue(__use);
        if (__use == __val) continue;
        __call->update(__use, __val);
    }
    // Assume the worst! No load store iff builtin.
    if (!__call->func->is_builtin) clearMemoryInfo();
}

void GlobalValueNumberPass::visitLoad(load_stmt *__load) {
    auto *__addr = __load->addr = getValue(__load->addr);
    auto *__dest = __load->dest;

    auto &__info = memMap[__addr];
    if (__info.val) defMap[__dest] = __info.val;
    else            __info.val     = __dest;

    /* Update the influence. */
    for (auto &[__def , __mem] : memMap) {
        if (__addr == __def) continue;
        if (mayAliasing(__addr, __def)) __mem.last = {}; // Last store may be used.
    }
}

void GlobalValueNumberPass::visitStore(store_stmt *__store) {
    auto *__src  = __store->src_ = getValue(__store->src_);
    auto *__addr = __store->addr = getValue(__store->addr);

    auto &__info = memMap[__addr];
    if (auto __last = __info.last)  deadStore.insert(__last);   // Useless last store.
    else if (__info.val == __src)   deadStore.insert(__store);  // Store loaded result.

    __info.last = __store;
    __info.val  = __src;

    /* Update the influence. */
    for (auto &[__def , __mem] : memMap) {
        if (__addr == __def) continue;
        if (__mem.val != __src && mayAliasing(__addr, __def))
            __mem = {}; // Reset this as useless.
    }
}

void GlobalValueNumberPass::visitGet(get_stmt *__get) {
    __get->addr  = getValue(__get->addr);
    __get->index = getValue(__get->index);
    auto [__iter, __success] = exprMap.try_emplace(__get, __get->dest);
    defMap[__get->dest] = __iter->second;
}


} // namespace dark::IR
