#pragma once
#include "IRbase.h"
#include <unordered_set>

namespace dark::IR {


/**
 * @brief Detects and removes unreachable blocks from the function
 */
struct unreachableRemover {
    unreachableRemover(function *);

    struct _Edge_t {
        block *from; block *to;
        friend bool operator == (const _Edge_t &, const _Edge_t &) = default;
    };
    struct _Edge_Hash {
        /**
         * Based on the intuition that one block 
         * has at most 2 successor, this is not
         * so bad as a hash function.
        */
        std::size_t operator()(const _Edge_t &e) const {
            return std::hash<block *>()(e.from);
        }
    };

  private:
    std::unordered_set <block *> visit0;
    std::unordered_set <block *> visit1;

    std::unordered_set <_Edge_t, _Edge_Hash> edges;

    void dfs0(block *); // Depth First Search
    void dfs1(block *); // Reverse Depth First Search
    void markUB(block *); // Find potentially unreachable blocks
    void updateCFG(block *);    // Update CFG
    void updatePhi(block *);    // Remove useless phi branches
    void recordCFG(block *);    // Record CFG
    void removeBlock(function *);   // Remove dead code
};

} // namespace dark::IR
