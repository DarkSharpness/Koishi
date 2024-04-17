#include "GVN/helper.h"
#include "GVN/algebraic.h"

namespace dark::IR::__gvn {

void algebraicSimplifier::visit(int __op, number_t __x, number_t __y) {
    using _Op_t = binary_stmt::binary_op;
    using enum _Op_t;
    switch (static_cast <_Op_t> (__op)) {
        case ADD: return visitADD(__x, __y);
        case SUB: return visitSUB(__x, __y);
        case MUL: return visitMUL(__x, __y);
        case DIV: return visitDIV(__x, __y);
        case MOD: return visitMOD(__x, __y);
        case SHL: return visitSHL(__x, __y);
        case SHR: return visitSHR(__x, __y);
        case AND: return visitAND(__x, __y);
        case OR:  return visitOR_ (__x, __y);
        case XOR: return visitXOR(__x, __y);
        default: runtime_assert(false, "Invalid operator.");
    }
}

} // namespace dark::IR::__gvn
