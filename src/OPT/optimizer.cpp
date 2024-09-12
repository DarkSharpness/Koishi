#include "optimizer.h"
#include "AST/folder.h"
#include "IRbase.h"
#include "IRbuilder.h"
#include "dominant.h"
#include "MEM2REG/mem2reg.h"
#include "DCE/unreachable.h"
#include "DCE/dce.h"
#include "DCE/adce.h"
#include "CP/sccp.h"
#include "GVN/gvn.h"
#include "LOOP/detector.h"
#include "CM/gcm.h"
#include "EARLY/gcr.h"
#include "CFGsimplifier.h"
#include <string_view>
#include <type_traits>

namespace dark {

static struct {
    std::size_t level = 2;
} optimize_info;

static void DoNotOptimize(IR::IRbuilder *ctx) {
    for (auto &__func : ctx->global_functions) {
        IR::unreachableRemover { &__func };
    }
}

template<typename _Pass, typename _Tp>
static void optimize(_Tp &object) {
    if constexpr (requires { _Pass { object }; }) {
        static_cast<void>(_Pass { object });
    } else {
        static_cast<void>(_Pass { &object });
    }
}

static void DoOptimize(IR::IRbuilder *ctx) {
    DoNotOptimize(ctx); // First, remove unreachable code.

    optimize<IR::GlobalConstantReplacer>(ctx);

    auto &functions = ctx->global_functions;
    for (auto &__func : functions) {
        optimize<IR::mem2regPass>(__func);
        optimize<IR::DeadCodeEliminator>(__func);
        optimize<IR::ConstantPropagatior>(__func);
        optimize<IR::dominantMaker>(__func);
        optimize<IR::unreachableRemover>(__func);
        optimize<IR::AggressiveElimination>(__func);
        optimize<IR::CFGsimplifier>(__func);

        optimize<IR::unreachableRemover>(__func);
        optimize<IR::unreachableRemover>(__func);
        optimize<IR::DeadCodeEliminator>(__func);
        optimize<IR::dominantMaker>(__func);
        optimize<IR::ConstantPropagatior>(__func);
        optimize<IR::unreachableRemover>(__func);
        optimize<IR::AggressiveElimination>(__func);
        optimize<IR::CFGsimplifier>(__func);

        optimize<IR::LoopNestDetector>(__func);
        optimize<IR::GlobalCodeMotionPass>(__func);
        optimize<IR::GlobalValueNumberPass>(__func);
        optimize<IR::DeadCodeEliminator>(__func);
        optimize<IR::ConstantPropagatior>(__func);
        optimize<IR::unreachableRemover>(__func);
        optimize<IR::DeadCodeEliminator>(__func);
    }

    optimize<IR::GlobalConstantReplacer>(ctx);

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
    if (argc != 0) {
        for (int i = 1 ; i < argc ; ++i) {
            const auto str = std::string_view { argv[i] };
            if (str == "-O2") {
                optimize_info.level = 2;
            } else if (str == "-O1") {
                optimize_info.level = 1;
            } else if (str == "-O0") {
                optimize_info.level = 0;
            } else {
                std::cerr << "Unknown option: " << str << std::endl;
            }
        }
    }
}

} // namespace dark
