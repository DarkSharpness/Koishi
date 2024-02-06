#pragma once
#include "IRbase.h"
namespace dark::IR {

struct ConstantCalculator : IRbase {
    using _List_t = std::vector <definition *>;
    definition *result {};
    ConstantCalculator(node *, const _List_t &);
  private:
    const _List_t &valueList;
    void visitCompare(compare_stmt *) override;
    void visitBinary(binary_stmt *) override;
    void visitJump(jump_stmt *) override;
    void visitBranch(branch_stmt *) override;
    void visitCall(call_stmt *) override;
    void visitLoad(load_stmt *) override;
    void visitStore(store_stmt *) override;
    void visitReturn(return_stmt *) override;
    void visitGet(get_stmt *) override;
    void visitPhi(phi_stmt *) override;
    void visitUnreachable(unreachable_stmt *) override;
    void setResult(definition *__def) { result = __def; }
};

} // namespace dark::IR
