#pragma once
#include "value.h"
#include "match.h"
#include "IRbase.h"

namespace dark::IR::__gvn {

struct algebraicSimplifier : private matcher {
  public:
    algebraicSimplifier() = default;
    void visit(int, number_t, number_t);
    std::variant <number_t, expression> result;
  private:

    void set_result(expression __e)  { result = __e; }
    void set_result(number_t __n)    { result = __n; }
    void set_result(int __op, number_t __x, number_t __y) {
        return set_result(expression {type_t::BINARY, __op, __x, __y});
    }

    bool is_negative(number_t __n, number_t &__x) const;

    void visitADD(number_t, number_t);  // +
    void visitSUB(number_t, number_t);  // -
    void visitNEG(number_t);            // -
    void visitMUL(number_t, number_t);  // *
    void visitDIV(number_t, number_t);  // /
    void visitMOD(number_t, number_t);  // %
    void visitSHL(number_t, number_t);  // <<
    void visitSHR(number_t, number_t);  // >>
    void visitAND(number_t, number_t);  // &
    void visitOR_(number_t, number_t);  // |
    void visitXOR(number_t, number_t);  // ^

    void updateCompare(compare_stmt *);
    void compareBoolean(compare_stmt *);
    void compareInteger(compare_stmt *);
};


} // namespace dark::IR::__gvn
