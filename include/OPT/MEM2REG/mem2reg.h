#pragma once
#include "IRbase.h"
#include <unordered_set>


namespace dark::IR {


struct mem2regPass {
    using _Block_List = std::vector <block *>;
    using _Node_List = std::vector <node *>;
    using _Map_t = std::unordered_map <local_variable *, phi_stmt *>;

    /**
     * @brief Mapping from a block to all phi functions
     * that will be inserted in that block.
     */
    std::unordered_map      <block *, _Map_t>           phiMap;
    /**
     * @brief Mapping from a local variable to its latest
     * definition. This is used to rename the variables.
    */
    std::unordered_map <local_variable *, definition *> varMap;
    /**
     * @brief Mapping from a temporary to all its uses.
    */
    std::unordered_map <temporary *, _Node_List>   useMap;

    _Block_List rpo;    // Reverse post order
    function *  top;    // Current function
    std::unordered_set <block *> visited;

    mem2regPass(function *);

  private:
    void spreadDef(block *);
    void spreadPhi();
    void rename(block *);
    void insertPhi(block *);
    void collectUse(block *);
    void visitBlock(block *);
    void spreadBranch(block *, block *);
};


} // namespace dark::IR
