#include "VN/gvn.h"
#include "IRnode.h"

namespace dark::IR {

/**
 * TODO: Spread the condition to the next block and 
 * its domSet.
 * Use a stack to maintain the condition.
 * like %x != 0, %y < 0.
 * This can be used to optimize the condition branch. 
 */
void GlobalValueNumberPass::visitBranch(branch_stmt *__br) {
    __br->cond = getValue(__br->cond);
}

void GlobalValueNumberPass::visitJump(jump_stmt *) {}
void GlobalValueNumberPass::visitUnreachable(unreachable_stmt *) {}
void GlobalValueNumberPass::visitReturn(return_stmt *__ret) {
    __ret->retval = getValue(__ret->retval); // Null case is safe.
}


} // namespace dark::IR
