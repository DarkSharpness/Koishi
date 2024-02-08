#include "VN/gvn.h"
#include "IRnode.h"

namespace dark::IR {

void GlobalValueNumberPass::visitCompare(compare_stmt *ctx) {
    nodeMap.try_emplace(ctx, true, ctx->op, ctx->lval, ctx->rval);
}


} // namespace dark::IR
