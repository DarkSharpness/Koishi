#pragma once
#include "IRbase.h"
#include <queue>
#include <unordered_set>

namespace dark::IR {

struct DeadCodeEliminator {
    std::unordered_set <statement *> effectSet;
    std::queue <statement *> workList;
    DeadCodeEliminator(function *);
};

} // namespace dark::IR
