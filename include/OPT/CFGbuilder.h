#pragma once
#include "IRnode.h"

namespace dark::IR {

/**
 * @brief Build the control flow graph for a function
*/
struct CFGbuilder {
    CFGbuilder(function *__func) {
        for (auto &__p : __func->data)
            __p->next.clear(), __p->prev.clear();
        for (auto &__p : __func->data) {
            if (auto *__br = __p->flow->as <branch_stmt> ())
                __link(__p, __br->branch[0]), __link(__p, __br->branch[1]);
            else if (auto *__jump = __p->flow->as <jump_stmt> ())
                __link(__p, __jump->dest);
        }
    }
  private:
    static void __link(block *__prev, block *__next) {
        __prev->next.push_back(__next);
        __next->prev.push_back(__prev);
    }
};

} // namespace dark::IR
