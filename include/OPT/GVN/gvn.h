#pragma once
#include "IRbase.h"
#include "memory.h"
#include "value.h"
#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include <deque>

namespace dark::IR::__gvn {

struct GlobalValueNumberPass final : IRbase {
  public:
    GlobalValueNumberPass(function *);
  private:
    using _Evec_t = std::deque <expression>;
    using _Emap_t = std::unordered_map <expression, equal_set, custom_hash>;
    using _Dmap_t = std::unordered_map <definition *, number_t>;
    using _Bset_t = std::unordered_set <block *>;

    /**
     * The index of each expression.
     * For each index, we maintian a set of definitions
     * that equals to the expression in value.
     * 
     * This map is used for hash-based CSE.
     * We may pick the best definition from the equal set.
    */
    _Emap_t cseMap;
    /**
     * The unique number of each definition.
     * Number = Type + Expression Pointer.
     * Number is the unique identifier for each definition.
    */
    _Dmap_t defMap;
    /**
     * A table storing all unknown expressions in the function.
     * We choose deque so that the address will not go invalid.
     */
    _Evec_t unknown;
    /**
     * Visited blocks.
     * This is just like a visit array in DFS.
     */
    _Bset_t visited;
    /**
     * A block-wise memory manager.
     * This manager can kill useless memory operations.
     */
    memorySimplifier memManager;

    [[nodiscard]]definition *pickBest(definition *);
    [[nodiscard]]definition *pickBest(number_t, typeinfo);

    number_t getNumber(definition *);

    void insertCSE(expression, temporary *);

    void visitGVN(block *);
    void removeHash(block *);
    void updateValue(temporary *, definition *);

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

    void setProperty(function *);
    bool checkProperty(function *);
    void removeDead(function *);
};


} // namespace dark::IR::__gvn

namespace dark::IR { using __gvn::GlobalValueNumberPass; }
