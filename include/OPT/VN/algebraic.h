#pragma once
#include "IRbase.h"

namespace dark::IR {

struct algebraicSimplifier {
  protected:
    definition *result {};

    using _Use_Map = std::unordered_map <temporary *, std::vector <node *>>;
    _Use_Map useMap;
    void setResult(definition *__def) { result = __def; }
    void collectUse(function *);

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
};


} // namespace dark::IR
