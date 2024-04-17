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
    using _Emap_t = std::unordered_map <expression, int , custom_hash>;
    using _Mset_t = std::unordered_set <store_stmt *>;
    using _Bset_t = std::unordered_set <block *>;
    using _Evec_t = std::vector <expression>;

    /* The expression reverse-mapping. */
    _Evec_t data;
    /* The number of value assigned to each expression. */
    _Emap_t numberMap;
    /* Block visit set. */
    _Bset_t visited;
    /* Set of dead stores. */
    _Mset_t deadStore;
    /* The memory simplifier. */
    memorySimplifier memManager;

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
