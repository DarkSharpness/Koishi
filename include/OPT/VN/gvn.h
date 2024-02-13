#pragma once
#include "IRbase.h"
#include "knowledge.h"
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

struct GlobalValueNumberPass final : IRbase {
  public:
    GlobalValueNumberPass(function *__func);
    static bool mayAliasing(definition *, definition *);
  private:

    using _Expr_Map = std::unordered_map <expression, definition *, expressionHash>;
    using _Pair_t   = typename _Expr_Map::value_type;
    using _Node_Map = std::unordered_map <node *, expression>;

    _Expr_Map exprMap;  // The expression in a simplified form.
    _Node_Map nodeMap;  // The original expression before modification.

    std::unordered_set <block *> visited;
    std::unordered_map <temporary *, definition *> defMap;
    std::unordered_map <non_literal *, size_t> useCount;

    struct memInfo {
        definition *val     {};
        store_stmt *last    {};
    };

    std::unordered_map <definition *, memInfo> memMap;
    std::unordered_set <store_stmt *> deadStore;


    definition *result {};
    void setResult(definition *__def) { result = __def; }
    static void makeDomTree(function *);

    void visitGVN(block *);
    void removeHash(block *);

    void visitAdd(binary_stmt *,definition *,definition *); // +
    void visitSub(binary_stmt *,definition *,definition *); // -
    void visitNeg(binary_stmt *,definition *);              // -
    void visitMul(binary_stmt *,definition *,definition *); // *
    void visitDiv(binary_stmt *,definition *,definition *); // /
    void visitMod(binary_stmt *,definition *,definition *); // %
    void visitShl(binary_stmt *,definition *,definition *); // <<
    void visitShr(binary_stmt *,definition *,definition *); // >>
    void visitAnd(binary_stmt *,definition *,definition *); // &
    void visitOr(binary_stmt *,definition *,definition *);  // |
    void visitXor(binary_stmt *,definition *,definition *); // ^

    void updateCompare(compare_stmt *);
    void compareBoolean(compare_stmt *);
    void compareInteger(compare_stmt *);

    void clearMemoryInfo();

    bool isAbsLess(definition *, definition *);
    bool isEqual(definition *, definition *);
    bool isNotEqual(definition *, definition *);
    bool isLessThan(definition *, definition *);
    bool isLessEqual(definition *, definition *);

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

    void collectUse(function *);
    void setProperty(function *);
    bool checkProperty(function *);

    definition *getValue(definition *);
};


} // namespace dark::IR
