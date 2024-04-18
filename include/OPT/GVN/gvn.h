#pragma once
#include "IRbase.h"
#include "memory.h"
#include "value.h"
#include <cstdint>
#include <unordered_set>
#include <unordered_map>

namespace dark::IR::__gvn {

struct GlobalValueNumberPass final : IRbase {
  public:
    GlobalValueNumberPass(function *);
  private:
    using _Emap_t = std::unordered_map <expression, int, custom_hash>;
    using _Dmap_t = std::unordered_map <definition *, number_t>;
    using _Bset_t = std::unordered_set <block *>;
    using _Evec_t = std::vector <expression>;

    /**
     * The expression of given index.
     * This pool is non-decreasing, which will only be appended.
    */
    _Evec_t data;
    /**
     * The index of each expression.
     * For each index, we maintian a set of definitions
     * that equals to the expression in value.
     * 
     * This map is used for hash-based CSE.
     * We may pick the best definition from the equal set.
    */
    _Emap_t numMap;
    /**
     * The unique number of each definition.
     * Number = Index + Type.
     * Number is the unique identifier for each definition.
    */
    _Dmap_t defMap;
    /* Block visit set. */
    _Bset_t visited;
    /* The memory simplifier. */
    memorySimplifier memManager;

    definition *pickBest(definition *);
    definition *pickBest(number_t);

    int unknown_count = 0;

    number_t getNumber(definition *);

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
