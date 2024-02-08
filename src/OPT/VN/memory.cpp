#include "VN/gvn.h"
#include "IRnode.h"

namespace dark::IR {

void GlobalValueNumberPass::visitCall(call_stmt *) {}

void GlobalValueNumberPass::visitLoad(load_stmt *) {}

void GlobalValueNumberPass::visitStore(store_stmt *) {}

void GlobalValueNumberPass::visitGet(get_stmt *) {}

} // namespace dark::IR
