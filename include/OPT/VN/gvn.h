#pragma once
#include "IRbase.h"
#include <unordered_set>

namespace dark::IR {

struct equalSet {
    /* A mapping from temporary to its representation */
    std::unordered_map <temporary *, definition *> defMap;
    definition *getValue(definition *__def) {
        auto __tmp = __def->as <temporary> ();
        if (!__tmp) return __def;
        return defMap.try_emplace(__tmp, __def).first->second;
    }
};

struct expression {
    bool    type;   // Binary/Compare
    int     op;     // Operator
    definition *lhs;
    definition *rhs;
    friend bool operator == (expression, expression) = default;
};

struct expression_hash {
    std::size_t operator () (const expression &__exp) const {
        std::hash <void *> __hash;
        std::size_t __base = int(__exp.type) << 4 | __exp.op;
        return __base ^ (__hash(__exp.lhs) + __hash(__exp.rhs));
    }
};

struct GlobalValueNumberPass final :  equalSet, IRbase {
  public:
    GlobalValueNumberPass(function *__func);
  private:
    using _Expr_Map = std::unordered_map <expression, definition *, expression_hash>;
    using _Pair_t   = typename _Expr_Map::value_type;
    using _Node_Map = std::unordered_map <node *, expression>;

    std::vector <block *> rpo;
    _Expr_Map exprMap;  // The expression in a simplified form.
    _Node_Map nodeMap;  // The original expression before modification.
    std::unordered_set <block *> visited;

    definition *result {};
    void setResult(definition *__def) { result = __def; }
    void makeDomTree();

    void visitGVN(block *);
    void updateNext(block *, block *);
    void removeHash(block *);

    void visitAdd(binary_stmt *,definition *,definition *); // +
    void visitSub(binary_stmt *,definition *,definition *); // -
    void visitMul(binary_stmt *,definition *,definition *); // *
    void visitDiv(binary_stmt *,definition *,definition *); // /
    void visitMod(binary_stmt *,definition *,definition *); // %
    void visitShl(binary_stmt *,definition *,definition *); // <<
    void visitShr(binary_stmt *,definition *,definition *); // >>
    void visitAnd(binary_stmt *,definition *,definition *); // &
    void visitOr(binary_stmt *,definition *,definition *);  // |
    void visitXor(binary_stmt *,definition *,definition *); // ^

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

};


} // namespace dark::IR
