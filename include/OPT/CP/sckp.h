#pragma once
#include "CP/scp.h"
#include "knowledge.h"

namespace dark::IR {

struct KnowledgePropagatior : SparseConditionalPropagatior, IRbase{
  public:
    KnowledgePropagatior(function *);

  private:
    struct defInfo : knowledge {
        enum defState { // Have 3 trials at most.
            UNDEFINED = 0,  // Best
            UNCERTAIN = 3,  // Worst
        };
        int type;
        bool is_const() const {
            return bits.valid == -1
                && size.upper == size.lower;
        }
        /* If constant, this is the constant value. */
        int value() const { return size.lower; }
        std::vector <statement *> useList;
    };

    std::unordered_map <temporary *, defInfo> defMap;
    std::unordered_map <block *, Blockinfo> blockMap;

    bool updated {};

    void initInfo(block *);
    void updateCFG(CFGinfo);
    void updateSSA(SSAinfo);
    void updatePhi(phi_stmt *);
    void updateStmt(statement *);
    void updateBranch(branch_stmt *);
    void tryUpdate(statement *);
    void modifyValue(block *);

    bool checkProperty(function *);
    void setProperty(function *);

    bitsInfo traceBits(definition *);
    sizeInfo traceSize(definition *);
    void updateSize(temporary *, sizeInfo);
    void updateBits(temporary *, bitsInfo);

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
};


} // namespace dark::IR
