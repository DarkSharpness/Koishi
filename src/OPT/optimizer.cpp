#include "optimizer.h"
#include "AST/folder.h"
#include "IRbuilder.h"
#include "dominant.h"

namespace dark {


void optimizer::init(int argc, char **argv) {
    // TODO: Complete parsing arguments.
}

void optimizer::optimize_AST(AST::ASTbuilder *ctx) {
    AST::constantFolder { ctx };
}

void optimizer::optimize_IR(IR::IRbuilder *ctx) {
    auto &functions = ctx->global_functions;
    for (auto &__func : functions) {
        IR::dominantMaker { &__func };
    }


}


} // namespace dark
