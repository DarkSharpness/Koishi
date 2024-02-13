#include "IRnode.h"
#include "EARLY/gcr.h"
#include "IRbuilder.h"


namespace dark::IR {

GlobalConstantReplacer::GlobalConstantReplacer(IRbuilder *builder) {
    for (auto &__func : builder->global_functions) scan(&__func);
    for (auto &__func : builder->global_functions) replace(&__func);
}

void GlobalConstantReplacer::scan(function *__func) {
    if (__func->name == "._Global_Init") return scanInit(__func);
    for (auto __block : __func->data)
        for (auto __stmt : __block->data)
            if (auto __store = __stmt->as <store_stmt> ())
                if (auto __var = __store->addr->as <global_variable> ())
                    useSet.insert(__var);
}

void GlobalConstantReplacer::scanInit(function *__init) {
    // Those only init once in global init function
    std::unordered_map <global_variable *, literal *> initSet;
    for (auto __block : __init->data)
        for (auto __stmt : __block->data)
            if (auto __store = __stmt->as <store_stmt> ())
                if (auto __var = __store->addr->as <global_variable> ()) {
                    if (auto __lit = __store->src_->as <literal> ()) {
                        auto [__iter, __success] =
                            initSet.try_emplace(__var, __lit);
                        if (__success) continue;
                    } useSet.insert(__var);
                }

    for (auto [__var, __lit] : initSet)
        if (useSet.count(__var) == 0) __var->init = __lit;
}



void GlobalConstantReplacer::replace(function *__func) {
    for (auto __block : __func->data)
        for (auto __stmt : __block->data)
            if (auto __load = __stmt->as <load_stmt> ())
                if (auto __var = __load->addr->as <global_variable> ())
                    if (useSet.count(__var) == 0)
                        valMap[__load->dest] =
                            const_cast <literal *> (__var->init);

    auto &&__replace_use = [this](statement *__stmt) -> void {
        for (auto __use : __stmt->get_use()) {
            auto __tmp = __use->as <temporary> ();
            if (!__tmp) return;
            auto __iter = valMap.find(__tmp);
            if (__iter != valMap.end()) {
                if (__iter->second == nullptr)
                    throw;

                __stmt->update(__tmp, __iter->second);
            }
        }
    };

    for (auto __block : __func->data) visitBlock(__block, __replace_use);

    valMap.clear(); // Clear the useless data.
}




} // namespace dark::IR
