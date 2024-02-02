#include "IRnode.h"
#include "dominant.h"
#include "MEM2REG/mem2reg.h"
#include <deque>
#include <memory>
#include <ranges>
#include <algorithm>
#include <functional>

namespace dark::IR {

/**
 * @brief Whether a local variable is simple and should be eliminated.
 */
static bool __is_simple(local_variable *__var) {
    // Only raw class objects are considered as simple.
    const auto __type = __var->type;
    return __type.dimensions > 1 || !dynamic_cast <const custom_type *> (__type.base);
};

static std::string __find_first_name(std::string_view __name) {
    __name.remove_prefix(1);
    return std::string(__name.substr(0, __name.find_first_of('-')));
}


mem2regPass::mem2regPass(function *__func) : top(__func) {
    dominantMaker dom {__func};
    if (__func->is_unreachable()) return;

    /* Step 1: Laying phi functions. */
    rpo = std::move(dom.rpo);
    for (auto *__block : rpo) spreadDef(__block);
    spreadPhi();

    /* Step 2: Update the local-var with latest value. */
    for (auto *__block : rpo) collectUse(__block);
    rename(__func->data[0]);
    for (auto *__block : rpo) insertPhi(__block);

    /* Step 3: Clean useless vars. */
    auto __first = __func->locals.begin();
    auto __last = std::ranges::copy(
        __func->locals | std::views::filter(std::not_fn(__is_simple)), __first).out;
    __func->locals.resize(__last - __first);
    dom.clean(__func);
}

/**
 * @brief Spread all store definition to the frontier.
 * Note that this function doesn't actually spread
 * those definitions.
 * It just reserve the phi functions in the frontier
 * for every stored variable.
 */
void mem2regPass::spreadDef(block *__node) {
    std::unordered_set <local_variable *> defs;
    for (auto __stmt : __node->data)
        if (auto __store = __stmt->as <store_stmt>())
            if (auto __var = __store->addr->as <local_variable>())
                if (__is_simple(__var))
                    defs.insert(__var);

    for (auto __next : getFrontier(__node)) {
        auto &__map = phiMap[__next];
        for (auto *__var : defs) {
            auto &__phi = __map[__var];
            if (!__phi) __phi = IRpool::allocate <phi_stmt> (nullptr);
        }
    }
}

/**
 * @brief Spread the phi functions to the frontier.
 * Similar to spreadDef, but this time we only
 * "spread" the phi functions newly generated to
 * the frontier.
 * This is a iterative process, so we use a queue.
 */
void mem2regPass::spreadPhi() {
    using _Add_List = std::vector <local_variable *>;
    using _Map_Iter = typename _Map_t::iterator;
    /* Mapping of all the nodes added. */
    std::unordered_map <block *, _Add_List> __added;
    for (auto &[__block, __map] : phiMap) {
        auto &__list = __added[__block];
        __list.resize(__map.size());
        std::ranges::copy(__map | std::views::keys, __list.begin());
    }
    /* Work list queue. */
    std::deque <block *> queue { rpo.begin() , rpo.end() };

    do { /* In queue iff __added[__node].size() > 0 */
        auto __node = queue.front(); queue.pop_back();
        auto __cur  = std::move(__added[__node]);

        /* Visit the frontier, tries to update. */
        for (auto __next : getFrontier(__node)) {
            std::size_t __cnt = 0;
            auto &__map = phiMap[__next];
            auto &__vec = __added[__next];
            for (auto __var : __cur) {
                auto &__phi = __map[__var];
                if (!__phi) {
                    __phi = IRpool::allocate <phi_stmt> (nullptr);
                    __vec.push_back(__var);
                    ++__cnt;
                }
            }
            if (__cnt != 0 && __cnt == __vec.size())
                queue.push_back(__next);
        }
    } while (!queue.empty());
}

void mem2regPass::rename(block *__node) {
    if (!visited.insert(__node).second) return; // Visited
    auto  __bak = std::make_unique <decltype(varMap)> (varMap);
    auto &__map = phiMap[__node];

    /* First, record those newly placed phi functions. */
    for (auto [__var, __phi] : __map) {
        __phi->dest = top->create_temporary(
            --__var->type, __find_first_name(__var->name));
        varMap[__var] = __phi->dest;
    }

    /* Then, goes to the body. */
    visitBlock(__node);

    /* Now, complete initialing those newly placed phi functions
        by setting the incoming values correspondingly. */
    for (auto __next : __node->next) {
        spreadBranch(__next , __node);
        rename(__next);
    }

    /* Finally, restore the varMap. */
    varMap = std::move(*__bak);
}

void mem2regPass::visitBlock(block *__node) {
    /* Key operation : load/store replacement. */
    auto __operation = [this](statement *__stmt) -> bool {
        if (auto *__store = __stmt->as <store_stmt>()) {
            if (auto *__var = __store->addr->as <local_variable>()) {
                if (__is_simple(__var)) {
                    varMap[__var] = __store->src_;
                    return false;
                }
            }
        } else if (auto *__load = __stmt->as <load_stmt>()) {
            if (auto *__var = __load->addr->as <local_variable>()) {
                if (__is_simple(__var)) {
                    auto *&__new = varMap[__var];
                    __load->addr = __new; 
                    runtime_assert(__new, "mem2reg: variable not initialized");
                    auto __iter = useMap.find(__load->dest);
                    if (__iter != useMap.end()) {
                        for (auto __tmp : __iter->second)
                            __tmp->update(__load->dest, __new);
                        useMap.erase(__iter);
                    }
                    return false;
                }
            }
        }
        return true;    // Can not be eliminated.
    };

    auto __first = __node->data.begin();
    auto __last = std::ranges::copy(
        __node->data | std::views::filter(__operation), __first).out;
    __node->data.resize(__last - __first);
}

void mem2regPass::spreadBranch(block *__node, block *__prev) {
    for (auto [__var, __phi] : phiMap[__node]) {
        auto *&__new = varMap[__var];
        /**
         * This is a special case of mem2reg.
         * It's not an error case. Indeed, it tells
         * that phi from this branch is not necessary.
         * We may optimize it out later.
        */
        if (!__new) __new = IRpool::create_undefined(--__var->type);
        __phi->list.push_back({__prev, __new});
    }
}

void mem2regPass::collectUse(block *__node) {
    auto __operation = [this](statement *__stmt) -> void {
        auto __vec = __stmt->get_use();
        for (auto __use : __vec)
            if (auto __var = __use->as <temporary>())
                useMap[__var].push_back(__stmt);
    };

    for (auto __stmt : __node->phi)
        __operation(__stmt->to_base());
    for (auto __stmt : __node->data) 
        __operation(__stmt->to_base());
    __operation(__node->flow->to_base());
}

void mem2regPass::insertPhi(block *__block) {
    std::ranges::copy(phiMap[__block] | std::views::values, std::back_inserter(__block->phi));
}

} // namespace dark::IR
