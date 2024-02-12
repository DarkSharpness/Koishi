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
#include "CP/sccp.h"
#include "VN/gvn.h"
#include "LOOP/detector.h"
#include "CP/sckp.h"
#include "CM/gcm.h"

namespace dark {

static struct {
    std::size_t level = 2;
} optimize_info;

static void DoNotOptimize(IR::IRbuilder *ctx) {
    for (auto &__func : ctx->global_functions) {
        IR::unreachableRemover { &__func };
    }
}

static void DoOptimize(IR::IRbuilder *ctx) {
    auto &functions = ctx->global_functions;
    for (auto &__func : functions) {
        IR::unreachableRemover { &__func };
        IR::mem2regPass { &__func };
        IR::DeadCodeEliminator { &__func };
        IR::ConstantPropagatior { &__func };
        IR::dominantMaker::clean(&__func);
        IR::unreachableRemover { &__func };
        IR::AggressiveElimination { &__func };

        IR::KnowledgePropagatior { &__func };
        IR::unreachableRemover { &__func };
        IR::DeadCodeEliminator { &__func };
        IR::dominantMaker { &__func };
        IR::ConstantPropagatior { &__func };
        IR::unreachableRemover { &__func };
        IR::AggressiveElimination { &__func };
        IR::LoopNestDetector { &__func };

        IR::KnowledgePropagatior { &__func };
        IR::GlobalCodeMotionPass { &__func };
        IR::GlobalValueNumberPass { &__func };
        IR::DeadCodeEliminator { &__func };
        IR::ConstantPropagatior { &__func };
        IR::unreachableRemover { &__func };


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
