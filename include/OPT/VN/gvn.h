#pragma once
#include "IRbase.h"
#include "knowledge.h"
#include "VN/mem.h"
#include "VN/algebraic.h"
#include <cstdint>
#include <unordered_set>

namespace dark::IR {

struct expression {
    enum {
        BINARY,     // binaray
        COMPARE,    // compare
        GETADDR,    // addr + offset
    };
    int     type;   // Expression type.
    int     op;     // Operator
    definition *lhs;
    definition *rhs;
    friend bool operator == (expression, expression) = default;

    expression(get_stmt *);
    expression(binary_stmt *);
    expression(compare_stmt *);
};

struct expressionHash {
    std::size_t operator () (const expression &__exp) const {
        std::hash <void *> __hash;
        std::size_t __base = int(__exp.type) << 4 | __exp.op;
        return __base ^ (__hash(__exp.lhs) + __hash(__exp.rhs));
    }
};

struct GlobalValueNumberPass final : IRbase , memorySimplifier, algebraicSimplifier {
  public:
    GlobalValueNumberPass(function *__func);
  private:

    using _Expr_Map = std::unordered_map <expression, definition *, expressionHash>;

    _Expr_Map exprMap;  // The expression in a simplified form.

    std::unordered_set <block *> visited;
    std::unordered_map <temporary *, definition *> defMap;

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

    static void makeDomTree(function *);
    void setProperty(function *);
    bool checkProperty(function *);
    void removeDead(function *);

};


} // namespace dark::IR
