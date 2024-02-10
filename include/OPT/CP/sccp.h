#pragma once
#include "CP/scp.h"

namespace dark::IR {

struct ConstantPropagatior : SparseConditionalPropagatior {
  public:
    ConstantPropagatior(function *);
  private:
    struct defInfo {
        definition *value;
        std::vector <statement *> useList;
    };
    std::vector <definition *> valueList;
    std::unordered_map <temporary *, defInfo> defMap;
    std::unordered_map <block *, Blockinfo> blockMap;

    const bool hasDomTree;

    definition *getValue(definition *);
    void initInfo(block *);
    void updateCFG(CFGinfo);
    void updateSSA(SSAinfo);
    void visitPhi(phi_stmt *);
    void visitStmt(statement *);
    void visitBranch(branch_stmt *);
    void tryUpdate(statement *);
    void modifyValue(block *);

    bool checkProperty(function *);
    void setProperty(function *);
};


} // namespace dark::IR
