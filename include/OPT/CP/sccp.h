#pragma once
#include "queue.h"
#include "IRbase.h"

namespace dark::IR {

struct ConstantPropagatior {
  public:
    ConstantPropagatior(function *);

  private:
    /* Mapping from a temporary to its potential new value. */
    using CFGinfo = struct { block *from, *to; };
    using SSAinfo = statement *;
    struct Blockinfo {
      private:
        std::vector <block *> visited;
      public:
        bool has_visit_from(block *__from) const {
            for (auto __node : visited) if (__node == __from) return true;
            return false;
        }
        bool try_visit(block *__from) {
            for (auto __node : visited) if (__node == __from) return false;
            return visited.push_back(__from), true;
        }
        std::size_t visit_count() const { return visited.size(); }
        bool is_first_visit() const { return visit_count() == 1; }
        bool is_not_visited() const { return visit_count() == 0; }
    };
    struct defInfo {
        definition *value;
        std::vector <statement *> useList;
    };

    std::vector <definition *> valueList;

    std::unordered_map <temporary *, defInfo> defMap;
    std::unordered_map <block *, Blockinfo> blockMap;

    std::queue <CFGinfo> CFGworklist;
    std::queue <SSAinfo> SSAworklist;

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
