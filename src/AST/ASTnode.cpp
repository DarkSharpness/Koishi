#include "ASTnode.h"


/* Some helper functions. */
namespace dark::AST {

static std::size_t global_indent = 0;

void print_indent() {
    for (std::size_t i = 0; i < global_indent; ++i)
        std::cout << "    ";
}

/**
 * @brief Print the statment after a field.
 * E.g. for, while, if, else, etc.
 */
void print_field(const statement *stmt) {
    if (auto __blk = dynamic_cast <const block_stmt *> (stmt)) {
        if (__blk->stmt.empty()) {
            std::cerr << "{}\n";
            return;
        }
        std::cerr << "{\n"; 

        ++global_indent;
        for (auto __p : __blk->stmt) {__p->print(); std::cout << '\n';}
        --global_indent;

        print_indent();
        std::cerr << "}\n";
    } else {
        std::cerr << '\n';

        ++global_indent;
        stmt->print();
        --global_indent;

        std::cerr << '\n';
    }
}



std::string typeinfo::data() const noexcept {
    std::string __str = base->name;
    for (int i = 0; i < dimensions; ++i) __str += "[]";
    return __str;
}

} // namespace dark::AST


/* Expression part. */
namespace dark::AST {

void subscript_expr::print() const {
    expr->print();
    for (auto __p : subscript) {
        std::cerr << "["; __p->print(); std::cerr << "]";
    }
}

void function_expr::print() const {
    func->print();
    std::cerr << "(";
    bool __first = true;
    for (auto __p : args) {
        if (__first) __first = false;
        else std::cerr << ", ";
        __p->print();
    }
    std::cerr << ")";
}

void unary_expr::print() const {
    if (op.str[2] != '\0') {
        expr->print();
        std::cerr << op.str[0] << op.str[1];
    } else {
        std::cerr << op.str;
        expr->print();
    }
}

void binary_expr::print() const {
    lval->print();
    std::cerr << " " << op.str << " ";
    rval->print();
}

void ternary_expr::print() const {
    cond->print();
    std::cerr << " ? ";
    lval->print();
    std::cerr << " : ";
    rval->print();
}

void member_expr::print() const {
    expr->print();
    std::cerr << '.' << name;
}

void construct_expr::print() const {
    std::cerr << "new " << type.base->name;
    std::size_t i = 0;
    for (; i != subscript.size(); ++i) {
        std::cerr << '[';
        subscript[i]->print();
        std::cerr << ']';
    }
    for (; (int)i != type.dimensions; ++i) std::cerr << "[]";
}

void atomic_expr::print()   const { std::cerr << name; }

void literal_expr::print()  const { std::cerr << name; }

} // namespace dark::AST

/* Statement part. */
namespace dark::AST {

void for_stmt::print() const {
    print_indent();
    std::cerr << "for (";
    size_t __temp = global_indent;
    global_indent = 0;

    if (init) init->print();
    std::cerr << ' ';
    global_indent = __temp;

    if (cond) cond->print();
    std::cerr << "; ";

    if (step) step->print();
    std::cerr << ") ";

    print_field(body);
}

void while_stmt::print() const {
    print_indent();
    std::cerr << "while (";
    cond->print();
    std::cerr << ") ";

    print_field(body);
}

void flow_stmt::print() const {
    print_indent();
    switch (type) {
        case flow_stmt::BREAK:      std::cerr << "break;";    return;
        case flow_stmt::CONTINUE:   std::cerr << "continue;"; return;
        default: /* Do nothing. */ break;
    }

    /* Return statement. */
    std::cerr << "return";
    if (expr) {
        std::cerr << ' ';
        expr->print();
    }
    std::cerr << ';';
}

void block_stmt::print() const {
    if (stmt.empty()) {
        std::cerr << "{}";
        return;
    }

    print_indent();
    std::cerr << "{\n";

    ++global_indent;
    for (auto __p : stmt) { __p->print(); std::cerr << '\n'; }
    --global_indent;

    print_indent();
    std::cerr << '}';
}

void branch_stmt::print() const {
    bool __first = true;
    for (auto [__cond,__body] : branches) {
        print_indent();
        if (__first) __first = false;
        else std::cerr << "else ";
        std::cerr << "if (";
        __cond->print();
        std::cerr << ") ";
        print_field(__body);
    }
    if (else_body) {
        print_indent();
        std::cerr << "else ";
        print_field(else_body);
    }
}

void simple_stmt::print() const {
    print_indent();
    bool __first = true;
    for (auto __p : expr) {
        __p->print();
        if (__first) __first = false;
        else std::cerr << ", ";
    }
    std::cerr << ';';
}

} // namespace dark::AST

namespace dark::AST {

void variable_def::print() const {
    print_indent();
    std::cerr << type.data() << ' ';
    bool __first = true;
    for (auto &&[__name, __expr] : vars) {
        if (__first) __first = false;
        else std::cerr << ", ";
        std::cerr << __name;
        if (__expr) {
            std::cerr << " = ";
            __expr->print();
        }
    }
    std::cerr << ';';
}

void function_def::print() const {
    print_indent();
    std::cerr << type.data() << ' ' << name << '(';
    bool __first = true;
    for (auto &&[__type, __name] : args) {
        if (__first) __first = false;
        else std::cerr << ", ";
        std::cerr << __type.data() << ' ' << __name;
    }
    std::cerr << ") ";
    if (!is_builtin()) print_field(body);
}

void class_def::print() const {
    print_indent();
    std::cerr << "class " << name << " {\n";

    ++global_indent;
    for (auto __p : member) { __p->print(); std::cerr << '\n'; }
    --global_indent;

    print_indent();
    std::cerr << '}';
}



} // namespace dark::AST
