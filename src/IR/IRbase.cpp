#include "IRbase.h"
#include "IRnode.h"
#include <deque>
#include <unordered_map>

namespace dark::IR {


typeinfo undefined::get_value_type() const { return type; }
std::string undefined::data() const {
    if (__is_bool_type(type))   return "false";
    if (__is_int_type(type))    return "0";
    return "null";
}


typeinfo string_constant::get_value_type() const { return {&stype}; }  
std::string string_constant::IRtype()   const {
    return std::format("private unnamed_addr constant {}", stype.name());
}
ssize_t string_constant::to_integer()   const {
    runtime_assert(false, "string_constant cannot be converted to integer"); 
    __builtin_unreachable();   
}
std::string string_constant::data()     const {
    std::string __ans = "c\"";
    for(char __p : context) {
        switch(__p) {
            case '\n': __ans += "\\0A"; break;
            case '\"': __ans += "\\22"; break;
            case '\\': __ans += "\\5C"; break;
            default: __ans.push_back(__p);
        }
    }
    __ans += "\\00\"";
    return __ans;
}
std::string string_constant::ASMdata()  const {
    std::string __ans = "\"";
    for(char __p : context) {
        switch(__p) {
            case '\n': __ans += "\\n"; break;
            case '\"': __ans += "\\\""; break;
            case '\\': __ans += "\\\\"; break;
            default: __ans.push_back(__p);
        }
    }
    __ans += "\"";
    return __ans;
}


std::string integer_constant::IRtype()  const { return "global i32"; }
ssize_t integer_constant::to_integer()  const { return value; }
std::string integer_constant::data()    const { return std::format("{}", value); }
typeinfo integer_constant::get_value_type() const { return {int_type::ptr()}; }


std::string boolean_constant::IRtype()  const { return "global i1"; }
ssize_t boolean_constant::to_integer()  const { return value; }
std::string boolean_constant::data()    const { return value ? "true" : "false"; }
typeinfo boolean_constant::get_value_type() const { return {bool_type::ptr()}; }


std::string pointer_constant::IRtype()  const { return "global ptr"; }
ssize_t pointer_constant::to_integer()  const {
    if (var == nullptr) return 0;
    runtime_assert(false, "pointer_constant cannot be converted to integer");
    __builtin_unreachable();
}
std::string pointer_constant::data()    const { return var ? var->name : "null"; }
typeinfo pointer_constant::get_value_type() const {
    return var ? ++var->type : typeinfo {null_type::ptr()};
}

} // namespace dark::IR

/* IR pool related part here. */
namespace dark::IR {

void IRpool::init_pool() {
    static bool __flag {};  // Avoid init twice.
    if (__flag) return;
    __dummy__.name  = ".$dummy";
    __flag      = true;
    __null__    = create_pointer(nullptr);
    __zero__    = create_integer(0);
    __pos1__    = create_integer(1);
    __neg1__    = create_integer(-1);   
    __true__    = create_boolean(true);
    __false__   = create_boolean(false);
}

integer_constant *IRpool::create_integer(int __x) {
    static std::unordered_map <int, integer_constant> __pool;
    return &__pool.try_emplace(__x, __x).first->second;
}

boolean_constant *IRpool::create_boolean(bool __x) {
    static boolean_constant __pool[2] {
        boolean_constant {false}, boolean_constant {true}
    };
    return &__pool[__x];
}

pointer_constant *IRpool::create_pointer(const global_variable *__x) {
    static std::unordered_map <const void *, pointer_constant> __pool;
    return &__pool.try_emplace(__x, __x).first->second;
}

/**
 * Hint = 0: No hint (default).
 * Hint = 1: Integer undefined.
 * Hint = 2: Boolean undefined.
 * Hint = 3: Pointer undefined.
 */
undefined *IRpool::create_undefined(typeinfo __tp, int __hint) {
    static undefined __ptr  {{null_type::ptr()}};
    static undefined __int  {{int_type::ptr()}};
    static undefined __bool {{bool_type::ptr()}};
    if (__hint) {
        switch (__hint) {
            case 1: return &__int;
            case 2: return &__bool;
            case 3: return &__ptr;
        }
        runtime_assert(false, "Invalid hint for undefined");
        __builtin_unreachable();
    } else {
        if (__is_int_type(__tp))    return &__int;
        if (__is_bool_type(__tp))   return &__bool;
        return &__ptr;
    }
}

/* Return the global variable constantly initialized by given string. */
global_variable *IRpool::create_string(const std::string &__name) {
    static std::deque <string_constant> __strings; 

    auto [__iter, __result] = str_pool.try_emplace(__name);
    auto *__var = &__iter->second;

    if (!__result) return __var;

    __var->name = std::format("@.str.{}", str_pool.size());
    __var->type = { string_type::ptr(), 1 };

    __var->init = &__strings.emplace_back(__name);
    __var->is_constant = true;
    return __var;
}

namespace detail {

inline static std::deque    <block>     blocks {};
inline static std::vector   <block *> block_stack {};

} // namespace detail

template <>
unreachable_stmt *IRpool::allocate <unreachable_stmt> () {
    static unreachable_stmt __instance {};
    return &__instance;
}
template <>
block *IRpool::allocate <block> () {
    using namespace detail;
    if (block_stack.size()) {
        auto *__blk = block_stack.back();
        block_stack.pop_back();
        return __blk;
    } else {
        auto *__blk = &detail::blocks.emplace_back();
        __blk->name = std::format(".bb{}", blocks.size());
        return __blk;
    }
}
void IRpool::deallocate(block *__blk) {
    using namespace detail;
    __blk->data.clear();
    __blk->phi.clear();
    __blk->prev.clear();
    __blk->next.clear();
    __blk->comments.clear();
    __blk->flow = nullptr;
    block_stack.push_back(__blk);
}

/**
 * @return Data of a custom_type.
 */
std::string custom_type::data() const {
    std::vector <std::string> __ret;
    __ret.emplace_back(std::format("{} = type {{ ", name()));
    for (auto __p : layout) {
        __ret.emplace_back(__p.name());
        __ret.emplace_back(" , ");
    }
    if (member.size()) __ret.back() = " }";
    else               __ret.back().back() = '}';
    return join_strings(__ret);
}


} // namespace dark::IR
