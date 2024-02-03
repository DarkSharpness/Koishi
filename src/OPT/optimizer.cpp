#include "optimizer.h"
#include "AST/folder.h"
#include "IRnode.h"
#include "IRbuilder.h"
#include "dominant.h"
#include "DCE/unreachable.h"
#include "MEM2REG/mem2reg.h"
#include "DCE/dce.h"

namespace dark {

static void link(IR::block *__prev, IR::block *__next) {
    __prev->next.push_back(__next);
    __next->prev.push_back(__prev);
}
static void initEdge(IR::function *__func) {
    for (auto &__p : __func->data) {
        if (auto *__br = dynamic_cast <IR::branch_stmt *> (__p->flow)) {
            link(__p, __br->branch[0]);
            link(__p, __br->branch[1]);
        }
        else if (auto *__jump = dynamic_cast <IR::jump_stmt *> (__p->flow))
            link(__p, __jump->dest);
    }
}

static struct {
    std::size_t level = 2;

} optimize_info;

void optimizer::init(int argc, char **argv) {
    // TODO: Complete parsing arguments.
}

void optimizer::optimize_AST(AST::ASTbuilder *ctx) {
    AST::constantFolder { ctx };
}

void optimizer::optimize_IR(IR::IRbuilder *ctx) {
    /* No optimization case. */
    if (optimize_info.level == 0) {
        for (auto &__func : ctx->global_functions) {
            initEdge(&__func);
            IR::unreachableRemover { &__func };
        } return;
    }

    /* Normal optimization pass. */
    auto &functions = ctx->global_functions;
    for (auto &__func : functions) {
        IR::mem2regPass { &__func };
        IR::DeadCodeEliminator { &__func };



    }

    std::cout << ctx->IRtree();
}


} // namespace dark
