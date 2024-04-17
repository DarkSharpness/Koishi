#include "GVN/memory.h"
#include "IRnode.h"
#include <algorithm>

namespace dark::IR::__gvn {

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

static bool analyzeMember(non_literal *__lhs, non_literal *__rhs);
static bool mayAliasing(definition *__l, definition *__r);
static addressType traceAddress(non_literal *__var);

static addressType traceAddress(non_literal *__var) {
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

/**
 * @return A conservative analysis of whether
 * the two non-literal may be aliasing.
 * If there's no way to determine, return true.
 */
static bool mayAliasing(definition *__l, definition *__r) {
    if (__l == __r) return true;
    auto __lhs = __l->as <non_literal> ();
    auto __rhs = __r->as <non_literal> ();
    if (!__lhs || !__rhs) { warning("WTF???"); return true; }
    if (__lhs->type != __rhs->type) return false;

    using enum addressType;
    auto __ltype = traceAddress(__lhs);
    auto __rtype = traceAddress(__rhs);
    auto [__min, __max] = std::minmax(__ltype, __rtype);
    switch (__min) {
        case GLOBAL:    return false;   // Global can't be aliasing.
        case GETMEMBER: return __min == __max ? analyzeMember(__lhs, __rhs) : false;
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

static bool analyzeMember(non_literal *__lhs, non_literal *__rhs) {
    auto *__lget = __lhs->as <temporary> ()->def->as <get_stmt> ();
    auto *__rget = __rhs->as <temporary> ()->def->as <get_stmt> ();
    if (__lget->member != __rget->member) return false;
    return mayAliasing(__lget->addr, __rget->addr);
}

/**
 * @brief Analyze the influence of a function call.
 * If builtin, no extra load/store will be done.
 * We may assume that the function call will not
 * change the memory state.
 * 
 * Otherwise, we may need function attributes.
 * TODO: Add function attribute support.
 * @return The result of the function call.
 */
definition *memorySimplifier::analyzeCall(call_stmt *__call) {
    if (!__call->func->is_builtin) this->reset();
    return __call->dest;
}

/**
 * @brief Analyze the influence of a load statement.
 * @return The result of the load statement (loaded value).
 */
definition *memorySimplifier::analyzeLoad(load_stmt *__load) {
    auto *__addr = __load->addr;
    auto *__dest = __load->dest;
    definition *__ret = __dest;

    auto &__info = memMap[__addr];
    if (__info.val) __ret       = __info.val;
    else            __info.val  = __dest;

    /* Update the influence. */
    for (auto &[__def , __mem] : memMap) {
        if (__addr == __def) continue;
        if (mayAliasing(__addr, __def)) __mem.last = {}; // Last store may be used.
    }

    return __ret;   // Return the loaded value.
}

void memorySimplifier::analyzeStore(store_stmt *__store) {
    auto *__src  = __store->src_;
    auto *__addr = __store->addr;

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

void memorySimplifier::reset() { memMap.clear(); }

} // namespace dark::IR
