#pragma once
#include "IRbase.h"
#include <unordered_set>

namespace dark::IR {


/**
 * @brief Detects and removes unreachable blocks from the function
 */
struct unreachableRemover {
    unreachableRemover(function *);

  private:
    std::unordered_set <block *> visit0;
    std::unordered_set <block *> visit1;

    void dfs0(block *); // Depth First Search
    void dfs1(block *); // Reverse Depth First Search
    void markUB(block *); // Find potentially unreachable blocks
    void updateCFG(block *);    // Update CFG
    void updatePhi(block *);    // Remove useless phi branches
    void removeBlock(function *);   // Remove dead code
};

} // namespace dark::IR
