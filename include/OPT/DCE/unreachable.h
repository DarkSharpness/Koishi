#pragma once
#include "IRbase.h"
#include <unordered_set>

namespace dark::IR {


/**
 * @brief Detects and removes unreachable blocks from the function
 */
struct unreachableRemover {
    std::unordered_set <block *> visit0;
    std::unordered_set <block *> visit1;
    unreachableRemover(function *);

    void dfs0(block *); // Depth First Search
    void dfs1(block *); // Reverse Depth First Search
};

} // namespace dark::IR
