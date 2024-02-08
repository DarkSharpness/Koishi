#include "DCE/dce.h"
#include "IRnode.h"
#include <ranges>
#include <algorithm>

namespace dark::IR {


DeadCodeEliminator::DeadCodeEliminator(function *__func) {
    if (!checkProperty(__func)) return;

    auto &&__collect_effective = [this](statement *__stmt) -> void { 
        if (auto *__call = __stmt->as <call_stmt> ()) {
            if (__call->func->is_side_effective())
                workList.push(__stmt);
        } else if (__stmt->as <store_stmt> ()
                || __stmt->as <flow_statement> ())
            workList.push(__stmt);
    };
    for (auto *__block : __func->data) visitBlock(__block, __collect_effective);

    while (!workList.empty())
        for (auto *__use : workList.pop_value()->get_use())
            if (auto *__tmp = __use->as <temporary> ())
                workList.push(__tmp->def);

    auto &&__leave_effective = std::views::filter(
        [this](statement *__stmt) -> bool { return workList.contains(__stmt); });
    for (auto *__block : __func->data)
        updateBlock(__block, __leave_effective);

    setProperty(__func);
}

void DeadCodeEliminator::setProperty(function *) {}

bool DeadCodeEliminator::checkProperty(function *__func) {
    return !__func->is_unreachable();
}


} // namespace dark::IR
