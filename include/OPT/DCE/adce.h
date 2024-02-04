#pragma once
#include "queue.h"
#include "IRbase.h"

namespace dark::IR {

struct AggressiveElimination {
    AggressiveElimination(function *);
  private:
    work_queue <statement *> stmtList;
    work_queue  <block *>   blockList;
    void markEffect(block *);
    void spreadEffect();
    void removeDead(block *);
};

} // namespace dark::IR
