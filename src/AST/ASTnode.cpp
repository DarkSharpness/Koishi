#include "ASTnode.h"


/* Some helper functions. */
namespace dark::AST {

static std::size_t global_indent = 0;

[[nodiscard]]std::string_view
print_indent(std::size_t __n = global_indent) {
    static const char __buf[] = // 128 at most.
        "                                "
        "                                "
        "                                "
        "                                ";
    const auto __size = __n * 4;
    runtime_assert(__size < sizeof(__buf), "Too long indent. wtf are u writing?");    
    return std::string_view { __buf, __size };
}

/**
 * @brief Print the statment after a field.
 * E.g. for, while, if, else, etc.
 */
[[nodiscard]] std::string
print_field(const statement *stmt) {
    if (auto __blk = dynamic_cast <const block_stmt *> (stmt)) {
        if (__blk->stmt.empty()) return " {}";
        std::vector <std::string> __ret = { " {\n" };

        ++global_indent;
        for (auto __p : __blk->stmt) {
            __ret.emplace_back(__p->to_string());
            __ret.emplace_back("\n");
        }
        --global_indent;

        __ret.emplace_back(print_indent());
        __ret.emplace_back("}");
        return join_strings(__ret);
    } else {
        ++global_indent;
        auto __str = stmt->to_string();
        --global_indent;
        return '\n' + std::move(__str);
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

std::string bracket_expr::to_string() const {
    return std::format("({})", expr->to_string());
}

std::string subscript_expr::to_string() const {
    std::vector <std::string> __ret;
    __ret.reserve(3 * subscript.size() + 1);
    __ret.emplace_back(expr->to_string());
    for (auto __p : subscript) {
        __ret.emplace_back("[");
        __ret.emplace_back(__p->to_string());
        __ret.emplace_back("[");
    }
    return join_strings(__ret);
}

std::string function_expr::to_string() const {
    std::vector <std::string> __ret;
    __ret.reserve(2 * args.size() + 3);

    __ret.emplace_back(expr->to_string());
    __ret.emplace_back("(");
    bool __first = true;
    for (auto __p : args) {
        if (__first) __first = false;
        else __ret.emplace_back(", ");
        __ret.emplace_back(__p->to_string());
    }
    __ret.emplace_back(")");
    return join_strings(__ret);
}

std::string unary_expr::to_string() const {
    if (op.str[2] != '\0') { // Suffix ++ or --
        return expr->to_string() + op.str[0] + op.str[0];
    } else {
        return op.str + expr->to_string();
    }
}

std::string binary_expr::to_string() const {
    return std::format("{} {} {}",
        lval->to_string(), op.str, rval->to_string());
}

std::string ternary_expr::to_string() const {
    return std::format("{} ? {} : {}",
        cond->to_string(), lval->to_string(), rval->to_string());
}

std::string member_expr::to_string() const {
    return std::format("{}.{}", expr->to_string(), name);
}

std::string construct_expr::to_string() const {
    std::vector <std::string> __ret;

    __ret.emplace_back("new ");
    __ret.emplace_back(type.base->name);

    for (std::size_t i = 0; i != subscript.size(); ++i) {
        __ret.emplace_back("[");
        __ret.emplace_back(subscript[i]->to_string());
        __ret.emplace_back("]");
    }

    std::string __str;
    int i = int(subscript.size());
    __str.reserve((type.dimensions - i) << 1);
    while (i++ != type.dimensions) (__str += '[') += ']';
    __ret.emplace_back(std::move(__str));

    return join_strings(__ret);
}

std::string atomic_expr::to_string()   const { return name; }

std::string literal_expr::to_string()  const { return name; }

} // namespace dark::AST

/* Statement part. */
namespace dark::AST {

std::string for_stmt::to_string() const {
    size_t __temp = global_indent;
    global_indent = 0;
    std::string __init = init ? init->to_string() : "";
    global_indent = __temp;

    std::string __cond = cond ? cond->to_string() : "";
    std::string __step = step ? step->to_string() : "";

    return std::format("{}for ({} {} ; {}){}",
        print_indent(), __init, __cond, __step, print_field(body)
    );
}

std::string while_stmt::to_string() const {
    return std::format("{}while ({}){}",
        print_indent(), cond->to_string(), print_field(body));
}

std::string flow_stmt::to_string() const {
    if (sort != flow_stmt::RETURN) {
        return std::format("{}{}",
            print_indent(), sort == flow_stmt::BREAK ? "break" : "continue;");
    } else {
        return std::format("{}return{}{}{}",
            print_indent(),
            expr ? ' ' : ';',
            expr ? expr->to_string() : "\0",
            expr ? ';' : '\0');
    }
}

std::string block_stmt::to_string() const {
    if (stmt.empty()) return "";
    std::vector <std::string> __ret;
    __ret.reserve(stmt.size() * 2 + 4);

    __ret.emplace_back(print_indent());
    __ret.emplace_back("{\n");

    ++global_indent;
    for (auto __p : stmt) {
        __ret.emplace_back(__p->to_string());
        __ret.emplace_back("\n");
    }
    --global_indent;

    __ret.emplace_back(print_indent());
    __ret.emplace_back("}");
    return join_strings(__ret);
}

std::string branch_stmt::to_string() const {
    std::vector <std::string> __ret;
    __ret.reserve(4 * branches.size() + (else_body ? 2 : 0));

    __ret.emplace_back(std::format("{}if (", print_indent()));
    bool __first = true;
    for (auto [__cond,__body] : branches) {
        if (__first) {
            __first = false;
        } else {
            if (__ret.back().back() == '}')
                __ret.emplace_back(" else if (");
            else
                __ret.emplace_back(
                    std::format("\n{}else if (", print_indent()));
        }

        __ret.emplace_back(__cond->to_string());
        __ret.emplace_back(")");
        __ret.emplace_back(print_field(__body));
    }

    if (else_body) {
        if (__ret.back().back() == '}')
            __ret.emplace_back(" else");
        else
            __ret.emplace_back(
                std::format("\n{}else", print_indent()));
        __ret.emplace_back(print_field(else_body));
    }

    return join_strings(__ret);
}

std::string simple_stmt::to_string() const {
    std::vector <std::string> __ret;
    __ret.reserve(expr.size() * 2 + 1);

    __ret.emplace_back(print_indent());
    bool __first = true;
    for (auto __p : expr) {
        __ret.emplace_back(__p->to_string());
        if (__first) __first = false;
        else __ret.emplace_back(", ");
    }
    __ret.emplace_back(";");

    return join_strings(__ret);
}

} // namespace dark::AST

namespace dark::AST {

std::string variable_def::to_string() const {
    std::vector <std::string> __ret;

    __ret.emplace_back(print_indent());
    __ret.emplace_back(type.data() + ' ');

    bool __first = true;
    for (auto &&[__name, __expr] : vars) {
        if (__first) __first = false;
        else __ret.emplace_back(", ");
        __ret.emplace_back(__name);

        if (__expr) {
            __ret.emplace_back(" = ");
            __ret.emplace_back(__expr->to_string());
        }
    }
    __ret.emplace_back(";");

    return join_strings(__ret);
}

std::string function_def::to_string() const {
    std::vector <std::string> __ret;

    __ret.emplace_back(print_indent());
    if (name.empty())
        __ret.emplace_back("<constructor> (");
    else
        __ret.emplace_back(type.data() + ' ' + name + '(');

    bool __first = true;
    for (auto &&[__type, __name] : args) {
        if (__first) __first = false;
        else __ret.emplace_back(", ");
        __ret.emplace_back(__type.data() + ' ' + __name);
    }
    __ret.emplace_back(")");
    if (!is_builtin()) __ret.emplace_back(print_field(body));
    return join_strings(__ret);
}

std::string class_def::to_string() const {
    std::vector <std::string> __ret;

    __ret.emplace_back(std::format("class {} {{\n", name));

    ++global_indent;
    for (auto __p : member) {
        __ret.emplace_back(__p->to_string());
        __ret.emplace_back("\n");
    } 
    --global_indent;
    __ret.emplace_back("};\n");
    return join_strings(__ret);
}



} // namespace dark::AST
