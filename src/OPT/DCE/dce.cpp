#include "DCE/dce.h"
#include "IRnode.h"
#include <ranges>
#include <algorithm>

namespace dark::IR {


DeadCodeEliminator::DeadCodeEliminator(function *__func) {
    if (__func->is_unreachable()) return;
    auto &&__collect_effective = [this](statement *__stmt) -> void { 
        if (auto *__call = __stmt->as <call_stmt> ()) {
            if (__call->func->is_side_effective()) {
                effectSet.insert(__stmt);
                workList.push(__stmt);
            }
        } else if (__stmt->as <store_stmt> ()
                || __stmt->as <flow_statement> ()) {
            effectSet.insert(__stmt);
            workList.push(__stmt);
        }
    };
    auto &&__leave_effective = std::views::filter([this](statement *__stmt) -> bool {
        return effectSet.contains(__stmt);
    });

    for (auto *__block : __func->data)
        visitBlock(__block, __collect_effective);

    while (!workList.empty()) {
        auto *__stmt = workList.front(); workList.pop();
        for (auto *__use : __stmt->get_use()) {
            if (auto *__tmp = __use->as <temporary> ()) {
                if (effectSet.insert(__tmp->def).second)
                    workList.push(__tmp->def);
            }
        }
    }

    for (auto *__block : __func->data) updateBlock(__block, __leave_effective);
}


} // namespace dark::IR
