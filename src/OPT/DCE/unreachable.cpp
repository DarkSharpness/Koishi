#include "DCE/unreachable.h"
#include "IRnode.h"
#include <ranges>
#include <algorithm>

namespace dark::IR {

unreachableRemover::unreachableRemover(function *__func) {
    if (__func->is_unreachable()) return;

    dfs0(__func->data[0]);
    for (auto *__p : __func->data)
        if (dynamic_cast <return_stmt *> (__p->flow))
            dfs1(__p);

    auto &&__range = __func->data |
        std::views::filter(
            [__set0 = std::move(visit0), 
             __set1 = std::move(visit1)](block *__p) -> bool {
                return __set0.count(__p) && __set1.count(__p);
            });

    auto __iter = __func->data.begin();
    __func->data.resize(std::ranges::copy(__range, __iter).out - __iter);
}

void unreachableRemover::dfs0(block *__p) {
    if (visit0.insert(__p).second)
        for (auto *__q : __p->next) dfs0(__q);
}

void unreachableRemover::dfs1(block *__p) {
    if (visit1.insert(__p).second)
        for (auto *__q : __p->prev) dfs1(__q);
}

} // namespace dark::IR