#include "CFGsimplifier.h"
#include "IRnode.h"
#include <ranges>

namespace dark::IR {

CFGsimplifier::CFGsimplifier(function *__func) {
    if (__func->is_unreachable()) return;
    dfs(__func->data[0]);
    for (auto __block : __func->data) update(__block);
    remove(__func);
}

void CFGsimplifier::dfs(block *__block) {
    if (!remap.try_emplace(__block, __block).second) return;
    while (__block->next.size() == 1) {
        auto __next = __block->next[0];
        if (__next->prev.size() == 1 && __next->phi.empty()) {
            __block->next = __next->next;
            __block->flow = __next->flow;
            __block->data.insert(__block->data.end(),
                __next->data.begin(), __next->data.end());
            remap.try_emplace(__next, __block);
            IRpool::deallocate(__next);
        } else break;
    }

    for (auto __next : __block->next) dfs(__next);
}

void CFGsimplifier::update(block *__block) {
    for (auto __phi : __block->phi)
        for (auto &[__prev, __] : __phi->list)
            __prev = remap[__prev];
    for (auto &__prev : __block->prev)
        __prev = remap[__prev];
}


void CFGsimplifier::remove(function *__func) {
    auto &&__range = __func->data | std::views::filter(
        [this](block *__block) { return remap[__block] == __block; }
    );
    auto __first = __func->data.begin();
    __func->data.resize(std::ranges::copy(__range, __first).out - __first);
}

} // namespace dark::IR
