#include "IRnode.h"

/* Node::data() */
namespace dark::IR {

using _Def_List = typename node::_Def_List;

template <typename _Range>
static std::string make_arglist(_Range &&__list) {
    std::string __ret; // Arguments.
    bool __first = true;
    for (auto *__arg : __list) {
        if (__first) __first = false;
        else __ret += ", ";
        __ret += __arg->get_value_type().name();
        __ret += ' ';
        __ret += __arg->data();
    }
    return __ret;
}

std::string compare_stmt::data() const {
    static const char *__map[] = {
        [EQ] = "eq",
        [NE] = "ne",
        [LT] = "slt",
        [LE] = "sle",
        [GT] = "sgt",
        [GE] = "sge",
    };

    runtime_assert(
        lval->get_value_type() == rval->get_value_type() &&
        dest->get_value_type() == typeinfo{ bool_type::ptr() },
        "Invalid compare operation type"
    );

    return std::format(
        "{} = icmp {} {} {}, {}\n",
        dest->data(),
        __map[op],
        lval->get_value_type().name(),
        lval->data(),
        rval->data()
    );
}

std::string binary_stmt::data() const {
    static const char *__map[] = {
        [ADD] = "add",
        [SUB] = "sub",
        [MUL] = "mul",
        [DIV] = "sdiv",
        [MOD] = "srem",
        [SHL] = "shl",
        [SHR] = "ashr",
        [AND] = "and",
        [OR]  = "or",
        [XOR] = "xor",
    };

    runtime_assert(
        lval->get_value_type() == rval->get_value_type() &&
        lval->get_value_type() == dest->get_value_type(),
        "Invalid binary operation type"
    );

    return std::format(
        "{} = {} {} {}, {}\n",
        dest->data(),
        __map[op],
        lval->get_value_type().name(),
        lval->data(),
        rval->data()
    );
}

std::string jump_stmt::data() const {
    return std::format("br label %{}\n", dest->name);
}

std::string branch_stmt::data() const {
    return std::format(
        "br i1 {}, label %{}, label %{}\n",
        cond->data(),
        branch[1]->name,
        branch[0]->name
    );
}

std::string call_stmt::data() const {
    std::string __list = make_arglist(args);
    std::string __prefix; // Prefix.
    if (dest) __prefix = std::format("{} =", dest->data());
    return std::format(
        "{} call {} @{}({})\n",
        __prefix,
        func->type.name(),
        func->name,
        __list
    );
}

std::string load_stmt::data() const {
    runtime_assert(
        dest->get_value_type() == addr->get_point_type(),
        "Invalid load operation type"
    );
    return std::format(
        "{} = load {}, ptr {}\n",
        dest->data(),
        dest->get_value_type().name(),
        addr->data()
    );
}

std::string store_stmt::data() const {
    runtime_assert(
        addr->get_point_type() == src_->get_value_type(),
        "Invalid store operation type"
    );
    return std::format(
        "store {}, ptr {}\n",
        src_->data(),
        addr->data()
    );
}

std::string return_stmt::data() const {
    runtime_assert(retval && retval->get_value_type() == func->type,
        "Invalid return type");
    runtime_assert(!retval && func->type == typeinfo{ void_type::ptr() },
        "Invalid return type");
    if (!retval) return "ret void\n";
    return std::format(
        "ret {} {}\n",
        func->type.name(),
        retval->data()
    );
}

std::string alloca_stmt::data() const {
    return std::format(
        "{} = alloca {}\n",
        dest->data(),
        dest->get_point_type().name()
    );
}

std::string get_stmt::data() const {
    runtime_assert(
        index->get_value_type() == typeinfo{ int_type::ptr() },
        "Invalid getelementptr index type"
    );

    std::string __suffix;
    if (member != NPOS) __suffix = std::format(", i32 {}", member);
    return std::format(
        "{} = getelementptr {}, ptr {}, i32 {} {}\n",
        dest->data(),
        addr->get_point_type().name(),
        addr->data(),
        index->data(),
        __suffix
    );
}

std::string unreachable_stmt::data() const { return "unreachable\n"; }

std::string phi_stmt::data() const {
    std::vector <std::string> __ret;

    __ret.reserve(list.size() * 2 + 1);
    __ret.push_back(std::format(
        "{} = phi {} ",
        dest->data(),
        dest->get_value_type().name()
    ));

    bool __first = true;
    for(auto [from, init] : list) {
        runtime_assert(
            dest->get_value_type() == init->get_value_type(),
            "Invalid phi statement!"
        );
        if (__first) __first = false;
        else __ret.push_back(", ");
        __ret.push_back(std::format("[{}, %{}]", init->data(), from->name));
    }

    __ret.push_back("\n");
    return join_strings(__ret);
}

} // namespace dark::IR

/* Defs and uses. */
namespace dark::IR {


temporary *compare_stmt::get_def() const { return dest; }
_Def_List  compare_stmt::get_use() const { return { lval, rval }; }
temporary *binary_stmt::get_def() const { return dest; }
_Def_List  binary_stmt::get_use() const { return { lval, rval }; }
temporary *jump_stmt::get_def() const { return nullptr; }
_Def_List jump_stmt::get_use() const { return {}; }
temporary *branch_stmt::get_def() const { return nullptr; }
_Def_List branch_stmt::get_use() const { return { cond }; }
temporary *call_stmt::get_def() const { return dest; }
_Def_List  call_stmt::get_use() const { return args; }
temporary *load_stmt::get_def() const { return dest; }
_Def_List  load_stmt::get_use() const { return { addr }; }
temporary *store_stmt::get_def() const { return nullptr; }
_Def_List  store_stmt::get_use() const { return { addr, src_ }; }
temporary *return_stmt::get_def() const { return nullptr; }
_Def_List  return_stmt::get_use() const { return { retval }; }
temporary *alloca_stmt::get_def() const { return nullptr; }
_Def_List  alloca_stmt::get_use() const { return {}; }
temporary *get_stmt::get_def() const { return dest; }
_Def_List  get_stmt::get_use() const { return { addr, index }; }
temporary *unreachable_stmt::get_def() const { return nullptr; }
_Def_List  unreachable_stmt::get_use() const { return {}; }
temporary *phi_stmt::get_def() const { return dest; }
_Def_List  phi_stmt::get_use() const {
    _Def_List __ret;
    __ret.reserve(list.size());
    for (auto [__, __init] : list) __ret.push_back(__init);
    return __ret;
}

}

/* Some other... */
namespace dark::IR {

void block::push_back(statement *__stmt) { data.push_back(__stmt); }
void block::print(std::ostream &os) const {
    os << name << ":\n";
    for (auto *__p : phi)   os << __p->data();
    for (auto *__p : data)  os << __p->data();
    os << '\n';
}

bool block::is_unreachable() const {
    return dynamic_cast <unreachable_stmt *> (flow);
}

temporary *function::create_temporary(typeinfo __tp, const std::string &__str) {
    auto *__temp = IRpool::allocate_def <temporary> ();
    __temp->type = __tp;
    __temp->name = std::format("%{}-{}", __str, temp_count[__str]++);
    return __temp;
}

void function::push_back(block *__blk) { data.push_back(__blk); }
void function::push_back(statement *__stmt) { data.back()->push_back(__stmt); }
void function::print(std::ostream &os) const {
    std::string __list = make_arglist(args);
    os << std::format(
        "define {} @{}() {{\n",
        type.name(), name 
    );
}

bool function::is_unreachable() const { return data.empty(); }

namespace detail {

static function __builtin_function[17] {};

} // namespace detail


/**
 * __Array_size__       = 0
 * strlen               = 1
 * __String_substring__ = 2
 * __String_parseInt__  = 3
 * __String_ord__       = 4
 * __print__            = 5
 * puts                 = 6
 * __printInt__         = 7
 * __printlnInt__       = 8
 * __getString__        = 9
 * __getInt__           = 10
 * __toString__         = 11
 * __string_add__       = 12
 * strcmp               = 13
 * malloc               = 14
 * __new_array1__       = 15
 * __new_array4__       = 16
*/
function *const IRpool::builtin_function = detail::__builtin_function;


} // namespace dark::IR
