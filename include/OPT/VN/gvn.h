#pragma once
#include "IRbase.h"
#include <cstdint>
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

struct expressionHash {
    std::size_t operator () (const expression &__exp) const {
        std::hash <void *> __hash;
        std::size_t __base = int(__exp.type) << 4 | __exp.op;
        return __base ^ (__hash(__exp.lhs) + __hash(__exp.rhs));
    }
};


struct sizeInfo {
    int upper = INT32_MAX;  // Maximum possible value.
    int lower = INT32_MIN;  // Minimum possible value.
    sizeInfo() = default;
    explicit sizeInfo(int __val) { upper = lower = __val; }
    explicit sizeInfo(long long __top, long long __low) {
        if (__top > INT32_MAX) __top = INT32_MAX;
        if (__low < INT32_MIN) __low = INT32_MIN;
        upper = __top;
        lower = __low;
        runtime_assert(upper >= lower, "Invalid sizeInfo");
    }
};

struct bitsInfo {
    int top  = 31;   // 0 ~ 31. Maximum possible bit.
    int low  = 0;    // 0 ~ 31. Minimum possible bit.
    bitsInfo() = default;
    explicit bitsInfo(int __val) {
        if (__val == 0) { top = -1; low = 32; return; }
        top = 31 - std::countl_zero((unsigned)__val);
        low = std::countr_zero((unsigned)__val);
    }
    explicit bitsInfo(int __top, int __low) { top = __top; low = __low; }
};

struct knowledge {
    sizeInfo size;
    bitsInfo bits;
    void init(sizeInfo);
    void init(bitsInfo);
};

struct GlobalValueNumberPass final : equalSet, IRbase {
  public:
    GlobalValueNumberPass(function *__func);
  private:
    using _Expr_Map = std::unordered_map <expression, definition *, expressionHash>;
    using _Pair_t   = typename _Expr_Map::value_type;
    using _Node_Map = std::unordered_map <node *, expression>;
    using _Info_Map = std::unordered_map <definition *, knowledge>;

    std::vector <block *> rpo;
    _Expr_Map exprMap;  // The expression in a simplified form.
    _Node_Map nodeMap;  // The original expression before modification.
    _Info_Map infoMap;  // The information of the definition.
    std::unordered_set <block *> visited;

    definition *result {};
    void setResult(definition *__def) { result = __def; }
    void makeDomTree();

    void visitGVN(block *);
    void removeHash(block *);

    void visitAdd(binary_stmt *,definition *,definition *); // +
    void visitSub(binary_stmt *,definition *,definition *); // -
    void visitNeg(binary_stmt *,definition *);             // -
    void visitMul(binary_stmt *,definition *,definition *); // *
    void visitDiv(binary_stmt *,definition *,definition *); // /
    void visitMod(binary_stmt *,definition *,definition *); // %
    void visitShl(binary_stmt *,definition *,definition *); // <<
    void visitShr(binary_stmt *,definition *,definition *); // >>
    void visitAnd(binary_stmt *,definition *,definition *); // &
    void visitOr(binary_stmt *,definition *,definition *);  // |
    void visitXor(binary_stmt *,definition *,definition *); // ^

    void updateAdd(binary_stmt *);
    void updateSub(binary_stmt *);
    void updateNeg(binary_stmt *);
    void updateMul(binary_stmt *);
    void updateDiv(binary_stmt *);
    void updateMod(binary_stmt *);
    void updateShl(binary_stmt *);
    void updateShr(binary_stmt *);
    void updateAnd(binary_stmt *);
    void updateOr(binary_stmt *);
    void updateXor(binary_stmt *);

    bool isAbsLess(definition *, definition *);
    bool isEqual(definition *, definition *);
    bool isNotEqual(definition *, definition *);
    bool isLessThan(definition *, definition *);
    bool isLessEqual(definition *, definition *);

    bitsInfo traceBits(definition *);
    sizeInfo traceSize(definition *);
    int traceSignBit(definition *);

    void buildKnowledge(binary_stmt *);

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
