#include "optimizer.h"
#include "AST/folder.h"
#include "IRnode.h"
#include "IRbuilder.h"
#include "dominant.h"
#include "CFGbuilder.h"
#include "MEM2REG/mem2reg.h"
#include "DCE/unreachable.h"
#include "DCE/dce.h"
#include "DCE/adce.h"

namespace dark {

static struct {
    std::size_t level = 2;

} optimize_info;

static void DoNotOptimize(IR::IRbuilder *ctx) {
    for (auto &__func : ctx->global_functions) {
        IR::CFGbuilder {&__func};
        IR::unreachableRemover { &__func };
    }
}

static void DoOptimize(IR::IRbuilder *ctx) {
    auto &functions = ctx->global_functions;
    for (auto &__func : functions) {
        IR::mem2regPass { &__func };
        IR::DeadCodeEliminator { &__func };
        IR::AggressiveElimination { &__func };
    }
}

void optimizer::optimize_AST(AST::ASTbuilder *ctx) {
    AST::constantFolder { ctx };
}

void optimizer::optimize_IR(IR::IRbuilder *ctx) {
    /* No optimization case. */
    optimize_info.level == 0 ? DoNotOptimize(ctx) : DoOptimize(ctx);
    std::cout << ctx->IRtree();
}


void optimizer::init(int argc, char **argv) {


}



} // namespace dark
