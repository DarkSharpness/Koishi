#include "DCE/unreachable.h"
#include "IRnode.h"
#include <ranges>
#include <algorithm>

namespace dark::IR {

unreachableRemover::unreachableRemover(function *__func) {
    if (__func->is_unreachable()) return;

    dfs0(__func->data[0]);
    for (auto *__p : __func->data)
        if (__p->flow->as <return_stmt> ()) dfs1(__p);

    removeBlock(__func);

    for (auto *__p : __func->data) updatePhi(__p);
}

void unreachableRemover::dfs0(block *__p) {
    if (visit0.insert(__p).second)
        for (auto *__q : __p->next) dfs0(__q);
}

void unreachableRemover::dfs1(block *__p) {
    if (visit1.insert(__p).second)
        for (auto *__q : __p->prev) dfs1(__q);
}

void unreachableRemover::updatePhi(block *__p) {
    for (auto *__phi : __p->phi) {
        auto &&__range = __phi->list |
            std::views::filter([this](phi_stmt::entry __e) -> bool {
                return visit0.count(__e.from) && visit1.count(__e.from);
            });
        auto __first = __phi->list.begin();
        __phi->list.resize(std::ranges::copy(__range, __first).out - __first);
    }
}

void unreachableRemover::removeBlock(function *__func) {
    auto &&__range = __func->data |
        std::views::filter([this](block *__p) -> bool {
            return visit0.count(__p) && visit1.count(__p);
        });
    auto __first = __func->data.begin();
    __func->data.resize(std::ranges::copy(__range, __first).out - __first);
}

} // namespace dark::IR
