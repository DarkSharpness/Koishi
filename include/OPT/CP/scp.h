#pragma once
#include "IRbase.h"
#include <queue>

namespace dark::IR {

struct SparseConditionalPropagatior {
  protected:
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

    std::queue <CFGinfo> CFGworklist;
    std::queue <SSAinfo> SSAworklist;
};


} // namespace dark::IR

