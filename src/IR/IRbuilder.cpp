#include "ASTnode.h"
#include "ASTscope.h"
#include "ASTbuilder.h"
#include "IRnode.h"
#include "IRbuilder.h"
#include <sstream>
#include <algorithm>

namespace dark::IR {

using _Phi_List = phi_stmt::_Phi_List;

static bool is_AST_string(AST::typeinfo __tp) { return __tp.base->name == "string" && __tp.dimensions == 0; }

/**
 * @brief Print out global variable with a given stream.
*/
static void print_global(const global_variable &__var, std::ostream &__ss) {
    __ss << __var.data();
    if (auto *__lit = __var.init)
        __ss << " = " << __lit->IRtype() << ' ' << __lit->data();
}

/**
 * @brief Initialize all builtin functions.
*/
static void init_builtin_functions() {
    using _Char_String = const char *;
    static constexpr _Char_String __name[] = {
        "__Array_size__",
        "strlen",
        "__String_substring__",
        "__String_parseInt__",
        "__String_ord__",
        "__print__",
        "puts",             // println
        "__printInt__",
        "__printlnInt__",
        "__getString__",
        "__getInt__",       // 10
        "__toString__",
        "__string_add__",
        "strcmp",
        "malloc",
        "__new_array1__",
        "__new_array4__",
    };

    constexpr auto __set_builtin_fn =
        [](std::size_t __n, typeinfo __tp,
           std::vector <argument *> __args) {
        auto *__func = IRpool::builtin_function + __n;
        __func->is_builtin = true;
        __func->type = __tp;
        __func->name = __name[__n];
        __func->args = std::move(__args);
    };

    typeinfo __int  = { int_type::ptr(), 0 };
    typeinfo __str  = { string_type::ptr(), 1 };
    typeinfo __nil  = { null_type::ptr(), 0 };
    typeinfo __void = { void_type::ptr(), 0 };

    auto *__int__ = IRpool::allocate <argument> ();
    __int__->type = __int;
    auto *__ptr__ = IRpool::allocate <argument> ();
    __ptr__->type = __nil;

    __set_builtin_fn(0, __int, { __ptr__ });
    __set_builtin_fn(1, __int, { __ptr__ });
    __set_builtin_fn(2, __str, { __ptr__, __int__ , __int__});
    __set_builtin_fn(3, __int, { __ptr__ });
    __set_builtin_fn(4, __int, { __ptr__, __int__ });
    __set_builtin_fn(5, __void, { __ptr__ });
    __set_builtin_fn(6, __void, { __ptr__ });
    __set_builtin_fn(7, __void, { __int__ });
    __set_builtin_fn(8, __void, { __int__ });
    __set_builtin_fn(9, __str,  {});
    __set_builtin_fn(10, __int, {});
    __set_builtin_fn(11, __str, { __int__ });
    __set_builtin_fn(12, __str, { __ptr__, __ptr__ });
    __set_builtin_fn(13, __int, { __ptr__, __ptr__ });
    __set_builtin_fn(14, __nil, { __int__ });
    __set_builtin_fn(15, __nil, { __int__ });
    __set_builtin_fn(16, __nil, { __int__ });
}

} // namespace dark::IR


namespace dark::IR {

/**
 * @return Requested value (not address form).
*/
definition *IRbuilder::get_value(){
    runtime_assert(is_ready, "No value is available.");
    set_invalid();
    if (!is_lvalue) return visit_value;
    /* We should load out the value now. */
    auto *__dest = top->create_temporary(visit_value->get_point_type(), "ld");
    top_block->push_back(IRpool::allocate <load_stmt> (
        __dest, safe_cast <non_literal *> (visit_value)));
    return __dest;
}

/**
 * @return Address of a left value.
 */
non_literal *IRbuilder::get_address() {
    runtime_assert(is_ready & is_lvalue, "Cannot get address of a rvalue.");
    set_invalid();
    return safe_cast <non_literal *> (visit_value);
}

branch_stmt *IRbuilder::get_branch() {
    runtime_assert(branch_stack.size(), "No branch is available.");
    return IRpool::allocate <branch_stmt> (get_value(), branch_stack.back().branch);
}

/**
 * @brief Set the value of a AST::variable.
 */
void IRbuilder::set_value(definition *__def) {
    visit_value = __def;
    is_lvalue   = false;
    is_ready    = true;
}

/**
 * @brief Set the address of a AST::variable.
 * It may be loaded later for the value.
 */
void IRbuilder::set_address(non_literal *__lit) {
    visit_value = __lit;
    is_lvalue   = true;
    is_ready    = true;
}

/* Set the value as invalid. */
void IRbuilder::set_invalid() { is_ready = false; }

/* Add a block to the back of the function. */
void IRbuilder::add_block(block *__blk) { top->data.push_back(top_block = __blk); }
/* End a block with a given flow. */
void IRbuilder::end_block(flow_statement *__stmt) {
    if (top_block != &dummy_block) {
        runtime_assert(top_block->flow == nullptr, "No flow is available.");        
        top_block->flow = __stmt;
        top_block = &dummy_block;
    } else { // Dummy block data is useless.
        dummy_block.data.clear();
    }
}


/**
 * @return IR in tree form, which can be compiled by clang.
 */
std::string IRbuilder::IRtree() const {
    std::stringstream ss;
    /* Class definitions. */
    for (auto &&[__str, __class] : class_map)
        if (auto *__ptr = dynamic_cast <custom_type *> (__class))
            ss << __ptr->data();
    ss << '\n';

    for (auto &&__var : global_variables)
        print_global(__var, ss);
    ss << '\n';

    for (auto &&__func : global_functions)
        __func.print(ss);
    ss << '\n';

    for (auto &&[__name, __var] : IRpool::str_pool)
        print_global(__var, ss);
    ss << '\n';

    return ss.str();
}

/**
 * @brief Tranform AST::typeinfo into IR::typeinfo
 * For non-trivial type, they are passed by pointer.
 * So we need to add one more dimension.
 * @return IR::typeinfo translated from AST::typeinfo.
 */
typeinfo IRbuilder::transform_type(AST::typeinfo __tp) {
    auto *__ptr = class_map[__tp.base->name];
    return { __ptr, __tp.dimensions + int(!__ptr->is_trivial())};
}

IRbuilder::IRbuilder(AST::ASTbuilder *ctx, AST::scope *__s) : global_scope(__s) {
    IRpool::init_pool();
    create_builtin(ctx);

    for (auto *__p : ctx->global)
        if (auto *__class = dynamic_cast <AST::class_def *> (__p))
            register_class(__class);

    for (auto *__p : ctx->global) {
        if (auto *__class = dynamic_cast <AST::class_def *> (__p))
            create_class(__class);
        else if (auto *__func = dynamic_cast <AST::function *> (__p))
            create_function(__func, false);
    }

    for (auto *__p : ctx->global) {
        top = nullptr; visit(__p);
    }
}

/**
 * @brief Just declare the existence of a class.
 */
void IRbuilder::register_class(AST::class_def *ctx) {
    auto *__class = &global_classes.emplace_back(std::format("%struct.{}", ctx->name));
    runtime_assert(class_map.try_emplace(ctx->name, __class).second,
                   "Class \"", ctx->name, "\" has been declared before.");
}

/**
 * @brief Build up a class.
 */
void IRbuilder::create_class(AST::class_def *ctx) {
    auto *__class = safe_cast <custom_type *> (class_map[ctx->name]);
    for (auto *__p : ctx->member) {
        if (auto *__func = dynamic_cast <AST::function_def *> (__p)) {
            create_function(__func, true);
            if (__func->name == "") {
                auto *__blk = safe_cast <AST::block_stmt *> (__func->body);
                /* Non-empty constructor function. */
                if (!__blk->stmt.empty()) {
                    /* Original name is "A." */
                    /* It will be transformed into "A.A" */
                    __class->constructor = &global_functions.back();
                    __class->constructor->name += ctx->name;
                }
            }
        } else { // Member variable case.
            auto *__var = safe_cast <AST::variable_def *> (__p);
            auto __type = transform_type(__var->type);

            for (auto &&[__name, __] : __var->vars) {
                __class->layout.push_back(__type);
                __class->member.push_back(__name);
                auto *__ptr = ctx->field->find(__name);
                variable_map[__ptr] = &member_variable;
            }
        }
    }
}

/**
 * @brief Build up global & member functions.
*/
void IRbuilder::create_function(AST::function_def *ctx, bool __is_member) {
    top = &global_functions.emplace_back();
    function_map[ctx] = top;
    top->type = transform_type(ctx->type);
    top->name = ctx->unique_name;

    auto *__entry = IRpool::allocate <block> ();
    top->push_back(__entry);

    /* Create all local variables. */
    for (auto *__p : ctx->locals) {
        auto *__var = IRpool::allocate <local_variable> ();
        __var->type = ++transform_type(__p->type);
        __var->name = __p->unique_name;
        variable_map[__p] = __var;
        top->locals.push_back(__var);
    }

    if (ctx->unique_name == ".main")
        return void((top->name = "main", main_function = top));

    /* Add "this" pointer for member functions. */
    if (__is_member) {
        auto *__this = ctx->field->find("this");
        runtime_assert(__this, "This shouldn't happen");
        auto *__var = IRpool::allocate <argument> ();
        __var->type = transform_type(__this->type);
        __var->name = "%this";
        top->args.push_back(__var);
    }

    /* Add normal function arguments to stack. */
    for (auto &&[__type, name] : ctx->args) {
        auto *__old = ctx->field->find(name);
        auto *__arg = IRpool::allocate <argument> ();
        __arg->type = transform_type(__type);
        __arg->name = __old->unique_name;
        top->args.push_back(__arg);

        /* The local variable used to store the function argument. */
        auto *__var = IRpool::allocate <local_variable> ();
        __var->type = ++__arg->type;
        __var->name = __arg->name + ".addr";
        top->locals.push_back(__var);
        variable_map[__old] = __var;

        __entry->push_back(IRpool::allocate <store_stmt> (__var, __arg));
    }
}

/**
 * @brief Create all the builtin functions required.
 * It also build up mapping from AST::function to IR::function
 * for those builtin global/member functions.
 */
void IRbuilder::create_builtin(AST::ASTbuilder *ctx) {
    class_map.try_emplace("string", string_type::ptr());
    class_map.try_emplace("int", int_type::ptr());
    class_map.try_emplace("void", void_type::ptr());
    class_map.try_emplace("null", null_type::ptr());
    class_map.try_emplace("bool", bool_type::ptr());

    auto * const builtin_function   = IRpool::builtin_function;

    builtin_function[5].has_output  = true;
    builtin_function[6].has_output  = true;
    builtin_function[7].has_output  = true;
    builtin_function[8].has_output  = true;
    builtin_function[9].has_input   = true; // getString
    builtin_function[10].has_input  = true; // getInt

    auto *_Array_scope  = ctx->classes["_Array"].field;
    auto *string_scope  = ctx->classes["string"].field;

    function_map[_Array_scope->find("size")]        = builtin_function + 0;
    function_map[string_scope->find("length")]      = builtin_function + 1;
    function_map[string_scope->find("substring")]   = builtin_function + 2;
    function_map[string_scope->find("parseInt")]    = builtin_function + 3;
    function_map[string_scope->find("ord")]         = builtin_function + 4;
    function_map[global_scope->find("print")]       = builtin_function + 5;
    function_map[global_scope->find("println")]     = builtin_function + 6;
    function_map[global_scope->find("printInt")]    = builtin_function + 7;
    function_map[global_scope->find("printlnInt")]  = builtin_function + 8;
    function_map[global_scope->find("getString")]   = builtin_function + 9;
    function_map[global_scope->find("getInt")]      = builtin_function + 10;
    function_map[global_scope->find("toString")]    = builtin_function + 11;

    return init_builtin_functions();
}


} // namespace dark::IR


namespace dark::IR {

void IRbuilder::visitBracket(AST::bracket_expr *ctx) { return visit(ctx->expr); }

void IRbuilder::visitSubscript(AST::subscript_expr *ctx) {
    visit(ctx->expr);
    for (auto *&__p : ctx->subscript) {
        auto *__ptr = safe_cast <non_literal *> (get_value());
        visit(__p);
        auto *__idx = get_value();  // Index
        auto *__tmp = top->create_temporary(__ptr->get_point_type(), "idx");
        auto *__get = IRpool::allocate <get_stmt> (__tmp, __ptr, __idx);
        top_block->push_back(__get);
        set_address(__tmp);
    }
}

void IRbuilder::visitFunction(AST::function_expr *ctx) {
    auto *__func = function_map[ctx->func];
    auto *__call = IRpool::allocate <call_stmt> (nullptr, __func);

    /* Member function, which needs to load this pointer. */
    if (ctx->func->unique_name[0] != '.') {
        visit(ctx->expr);
        __call->args.push_back(get_value());
    }
    for (auto *__arg : ctx->args) {
        visit(__arg);
        __call->args.push_back(get_value());
    }

    top_block->push_back(__call);

    if (__call->func->type != typeinfo { void_type::ptr(), 0 }) {
        __call->dest = top->create_temporary(__call->func->type, "call");
        set_value(__call->dest);
    } else { // Void type func call, no return value.
        __call->dest = nullptr;
        set_invalid();
    }
}

void IRbuilder::visitUnary(AST::unary_expr *ctx) {
    visit(ctx->expr);
    if (ctx->op.str[1]) {   // ++ or --
        /* Load out address ptr and value stored. */
        auto *__ptr = get_address();
        set_address(__ptr);
        auto *__val = get_value(); 

        bool  __add = ctx->op.str[1] == '+';
        auto *__tmp = top->create_temporary({ int_type::ptr() },
                                            __add ? "inc" : "dec");
        top_block->push_back(IRpool::allocate <binary_stmt> (
            __tmp, __val, IRpool::__pos1__, __add ? binary_stmt::ADD : binary_stmt::SUB));

        top_block->push_back(IRpool::allocate <store_stmt> (__tmp, __ptr));

        if (ctx->op.str[2]) set_value(__tmp);       // Suffix ++ or --
        else                set_address(__ptr);     // Prefix ++ or --
    } else {
        auto __op = ctx->op.str[0];
        if (__op == '+') return set_value(get_value());
        if (__op == '-') {
            auto *__dest = top->create_temporary({ int_type::ptr() }, "neg");
            top_block->push_back(IRpool::allocate <binary_stmt> (
                __dest, IRpool::__zero__, get_value(), binary_stmt::SUB));
            set_value(__dest);
        } else if (__op == '~') {
            auto *__dest = top->create_temporary({ int_type::ptr() }, "rev");
            top_block->push_back(IRpool::allocate <binary_stmt> (
                __dest, IRpool::__neg1__, get_value(), binary_stmt::XOR));
            set_value(__dest);
        } else if (__op == '!') {
            /* Branch case: Optimized. */
            if (branch_stack.size()) return branch_stack.back().negate();

            /* Non-branch case. */
            auto *__dest = top->create_temporary({ bool_type::ptr() }, "not");
            top_block->push_back(IRpool::allocate <compare_stmt> (
                __dest, get_value(), IRpool::__false__, compare_stmt::EQ));
            set_value(__dest);
        } else {
            runtime_assert(false, "Unknown unary operator.");
        }
    }
}

void IRbuilder::visitBinary(AST::binary_expr *ctx) {
    if (!ctx->op.str[1]) {
        if (ctx->op == "=")  return visitAssign(ctx);
        if (ctx->op == "<")  return visitCompare(ctx, compare_stmt::LT);
        if (ctx->op == ">")  return visitCompare(ctx, compare_stmt::GT);
    } else if (!ctx->op.str[2]) {
        if (ctx->op == "&&") return visitShortCircuit(ctx, false);
        if (ctx->op == "||") return visitShortCircuit(ctx, true);
        if (ctx->op == "!=") return visitCompare(ctx, compare_stmt::NE);
        if (ctx->op == "==") return visitCompare(ctx, compare_stmt::EQ);
        if (ctx->op == "<=") return visitCompare(ctx, compare_stmt::LE);
        if (ctx->op == ">=") return visitCompare(ctx, compare_stmt::GE);
    }
    /* String concatenation. */
    visit(ctx->lval);
    auto *__lval = get_value();
    visit(ctx->rval);
    auto *__rval = get_value();

    if (ctx->op.str[0] == '+' && is_AST_string(ctx->lval->type)) {
        auto *__func = IRpool::builtin_function + 12;
        auto *__dest = top->create_temporary(__func->type, "call");
        auto *__call = IRpool::allocate <call_stmt> (__dest, __func);
        __call->args = { __lval, __rval };
        top_block->push_back(__call);
        return set_value(__dest);
    }

    /**
     * @brief Only these cases are left behind:
     * + - * / % & | ^ << >>
    */
    binary_stmt::binary_op __op;
    const char *__msg;
    switch (ctx->op.str[0]) {
        case '+': __op = binary_stmt::ADD; __msg = "add"; break;
        case '-': __op = binary_stmt::SUB; __msg = "sub"; break;
        case '*': __op = binary_stmt::MUL; __msg = "mul"; break;
        case '/': __op = binary_stmt::DIV; __msg = "div"; break;
        case '%': __op = binary_stmt::MOD; __msg = "mod"; break;
        case '&': __op = binary_stmt::AND; __msg = "and"; break;
        case '|': __op = binary_stmt::OR;  __msg = "or";  break;
        case '^': __op = binary_stmt::XOR; __msg = "xor"; break;
        case '<': __op = binary_stmt::SHL; __msg = "shl"; break;
        case '>': __op = binary_stmt::SHR; __msg = "shr"; break;
        default: runtime_assert(false, "Unknown binary operator.");
    }

    auto *__tmp = top->create_temporary({ int_type::ptr() }, __msg);
    top_block->push_back(IRpool::allocate <binary_stmt> (__tmp, __lval, __rval, __op));
    return set_value(__tmp);
}

/* Simple assignment. */
void IRbuilder::visitAssign(AST::binary_expr *ctx) {
    visit(ctx->lval);
    auto *__addr = get_address();
    visit(ctx->rval);
    auto *__store = IRpool::allocate <store_stmt> (get_value(), __addr);
    top_block->push_back(__store);
    return set_address(__addr);
}

/**
 * @brief Short circuit evaluation.
 * __op = true:     || case
 * __op = false:    && case
*/
void IRbuilder::visitShortCircuit(AST::binary_expr *ctx, bool __op) {
    /* Normal short circuit. Use phi to collect the value. */
    if (branch_stack.empty()) return visitShortCircuitPhi(ctx, __op);
    /* Branch short circuit. Specially optimized. */
    auto *__branch_tmp  = IRpool::allocate <block> ();

    branch_stack.push_back(branch_stack.back());
    branch_stack.back().branch[!__op]  = __branch_tmp;

    visit(ctx->lval);

    end_block(get_branch());
    branch_stack.pop_back();

    add_block(__branch_tmp);
    return visit(ctx->rval);
}

/**
 * @brief Short circuit evaluation.
 * __op = true:     || case
 * __op = false:    && case
*/
void IRbuilder::visitShortCircuitPhi(AST::binary_expr *ctx, bool __op) {
    auto *__branch_true_    = IRpool::allocate <block> ();
    auto *__branch_false    = IRpool::allocate <block> ();
    auto *__branch_end      = IRpool::allocate <block> ();
    branch_stack.push_back({__branch_false, __branch_true_});

    visitShortCircuit(ctx, __op);

    end_block(get_branch());
    branch_stack.pop_back();

    add_block(__branch_true_);
    end_block(IRpool::allocate <jump_stmt> (__branch_end));

    add_block(__branch_false);
    end_block(IRpool::allocate <jump_stmt> (__branch_end));

    add_block(__branch_end);
    auto *__tmp = top->create_temporary({ bool_type::ptr() }, "phi");
    top_block->push_phi(IRpool::allocate <phi_stmt> (
        __tmp, _Phi_List {
            { __branch_false, IRpool::__false__ },
            { __branch_true_, IRpool::__true__  },
    }));
    return set_value(__tmp);
}

/* Compare operation. */
void IRbuilder::visitCompare(AST::binary_expr *ctx, int __input) {
    auto __op = compare_stmt::compare_op(__input);

    visit(ctx->lval);
    auto *__lval = get_value();

    visit(ctx->rval);
    auto *__rval = get_value();

    /* String comparation goes to call .strcmp first. */
    if (is_AST_string(ctx->lval->type)) {
        auto *__func = IRpool::builtin_function + 13;
        auto *__dest = top->create_temporary(__func->type, "call");
        auto *__call = IRpool::allocate <call_stmt> (__dest, __func);
        __call->args = { __lval, __rval };
        top_block->push_back(__call);
        __lval = __dest;
        __rval = IRpool::__zero__;
    }

    auto *__tmp = top->create_temporary({ bool_type::ptr() }, "cmp");
    top_block->push_back(IRpool::allocate <compare_stmt> (__tmp, __lval, __rval, __op));
    return set_value(__tmp);
}

void IRbuilder::visitTernary(AST::ternary_expr *ctx) {
    auto *__branch_true_    = IRpool::allocate <block> ();
    auto *__branch_false    = IRpool::allocate <block> ();
    branch_stack.push_back({__branch_false, __branch_true_});

    visit(ctx->cond);

    end_block(get_branch());
    branch_stack.pop_back();

    auto *__branch_end = IRpool::allocate <block> ();

    add_block(__branch_true_);
    visit(ctx->lval);
    auto *__lval = get_value();
    end_block(IRpool::allocate <jump_stmt> (__branch_end));

    add_block(__branch_false);
    visit(ctx->rval);
    auto *__rval = get_value();
    end_block(IRpool::allocate <jump_stmt> (__branch_end));

    add_block(__branch_end);
    auto *__tmp = top->create_temporary(__lval->get_point_type(), "phi");
    top_block->push_phi(IRpool::allocate <phi_stmt> (
        __tmp, _Phi_List {
        { __branch_false, __rval },
        { __branch_true_, __lval },
    }));
    return set_value(__tmp);
}

void IRbuilder::visitMember(AST::member_expr *ctx) {
    visit(ctx->expr);

    /* Member function. This pointer has been loaded through visit(ctx->expr) */
    if (ctx->type.is_function()) return;

    auto *__this    = get_value();
    auto  __type    = ++transform_type(ctx->type);
    auto *__dest    = top->create_temporary(__type, "get");
    auto *__class   = safe_cast <const custom_type *> (__this->get_value_type().base);

    top_block->push_back(IRpool::allocate <get_stmt> (
        __dest, __this, IRpool::__zero__, __class->get_index(ctx->name)));

    return set_address(__dest);
}

void IRbuilder::visitConstruct(AST::construct_expr *ctx) {
    std::vector <definition *> __subscript;
    for (auto *__p : ctx->subscript) {
        visit(__p);
        __subscript.push_back(get_value());
    }

    std::reverse(__subscript.begin(), __subscript.end());
    const auto __type = transform_type(ctx->type);
    if (__subscript.size())
        return visitNewArray(__type, std::move(__subscript));

    /**
     * Allocate exactly one object case.
     * Call malloc first.
     * If the class has a constructor, call it.
     */
    auto *__class = safe_cast <const custom_type *> (__type.base);
    auto *__func = IRpool::builtin_function + 14;
    auto *__dest = top->create_temporary(__func->type, "new");
    auto *__call = IRpool::allocate <call_stmt> (__dest, __func);
    __call->args = { IRpool::create_integer(__class->size()) };
    top_block->push_back(__call);

    /* Call the ctor. */
    if (auto *__ctor = __class->constructor) {
        auto *__call = IRpool::allocate <call_stmt> (nullptr, __ctor);
        __call->args = { __dest };
        top_block->push_back(__call);
    }

    return set_value(__dest);
}

/**
 * @brief Including 2 cases:
 *  1. Normal atomic.
 *  2. Implicit this member access.
 */
void IRbuilder::visitAtomic(AST::atomic_expr *ctx) {
    if (ctx->type.is_function()) {
        auto *__func = safe_cast <AST::function *> (ctx->real);
        /* Implicit member function. Return this pointer as first argument. */
        if (__func->field->find("this"))
            return set_value(top->args[0]);
        else /* Non-this pointer. */
            return set_invalid();
    }

    /* Explicit this pointer. */
    if (ctx->name == "this") return set_value(top->args[0]);

    auto *__var = variable_map[ctx->real];
    runtime_assert(__var, "wtf?");

    /* Implicit member variable access. Add offset. */
    if (__var == &member_variable) {
        auto *__this    = top->args[0];
        auto  __type    = ++transform_type(ctx->type);
        auto *__dest    = top->create_temporary(__type, "get");
        auto *__class   = safe_cast <const custom_type *> (__this->type.base);

        top_block->push_back(IRpool::allocate <get_stmt> (
            __dest, __this, IRpool::__zero__, __class->get_index(ctx->name)));
        return set_address(__dest);
    } else { /* Return local_variable (in stack.) */
        return set_address(__var);
    }
}

/* Literal constants. */
void IRbuilder::visitLiteral(AST::literal_expr *ctx) {
    switch (ctx->sort) {
        case ctx->NUMBER: return set_value(IRpool::create_integer(std::stoi(ctx->name)));
        case ctx->STRING: return set_value(IRpool::create_string(Mx_string_parse(ctx->name)));
        case ctx->_NULL_: return set_value(IRpool::__null__);
        case ctx->_BOOL_: return set_value(IRpool::create_boolean(ctx->name == "true"));
        runtime_assert(false, "Unknown literal type.");
        __builtin_unreachable();
    }
}

/**
 * @brief We should transfrom:
 *  "for (init; cond; step) body"
 * into:
 *  "init; if (cond) do { body; step; } while (cond)"
 */
void IRbuilder::visitFor(AST::for_stmt *ctx) {
    if (ctx->init) visit(ctx->init), set_invalid();

    auto *__loop_body   = IRpool::allocate <block> ();
    auto *__loop_step   = IRpool::allocate <block> ();
    auto *__loop_end    = IRpool::allocate <block> ();

    /* Capturing __loop_end, __loop_body, ctx, this. */
    auto __visit_cond = [=, this]() {
        if (ctx->cond) {
            branch_stack.push_back({__loop_end, __loop_body});

            visit(ctx->cond);

            end_block(get_branch());
            branch_stack.pop_back();
        } else { /* Directly jump to loop body. */
            end_block(IRpool::allocate <jump_stmt> (__loop_body));
        }
    };

    __visit_cond();

    add_block(__loop_body);
    loop_stack.push_back({__loop_step, __loop_end});
    visit(ctx->body);
    loop_stack.pop_back();

    end_block(IRpool::allocate <jump_stmt> (__loop_step));
    add_block(__loop_step);

    if (ctx->step) visit(ctx->step), set_invalid();
    __visit_cond();

    return add_block(__loop_end); // End of the loop.
}

/**
 * @brief We should transfrom:
 *  "while (cond) body"
 * into:
 *  "if (cond) do { body; } while (cond)"
 */
void IRbuilder::visitWhile(AST::while_stmt *ctx) {
    AST::for_stmt __ctx;
    __ctx.init = {};
    __ctx.cond = ctx->cond;
    __ctx.step = {};
    __ctx.body = ctx->body;
    return visitFor(&__ctx);
}

void IRbuilder::visitFlow(AST::flow_stmt *ctx) {
    if (ctx->sort != ctx->RETURN) {    // Loop case
        if (ctx->sort == ctx->CONTINUE)
            return end_block(IRpool::allocate <jump_stmt> (loop_stack.back().next));
        else    // Break case
            return end_block(IRpool::allocate <jump_stmt> (loop_stack.back().exit));
    }

    /* Return statement. */
    if (ctx->expr) {
        visit(ctx->expr);
        if (!__is_void_type(transform_type(ctx->expr->type)))
            return end_block(IRpool::allocate <return_stmt> (get_value(), top));
    }

    /* No return value. */
    return end_block(IRpool::allocate <return_stmt> (nullptr, top));
}

void IRbuilder::visitBlock(AST::block_stmt *ctx) {
    for (auto *__p : ctx->stmt) visit(__p), set_invalid();
}

void IRbuilder::visitSimple(AST::simple_stmt *ctx) {
    for (auto *__p : ctx->expr) visit(__p), set_invalid();
}

void IRbuilder::visitBranch(AST::branch_stmt *ctx) {
    auto *__branch_exit = IRpool::allocate <block> ();
    for (const auto &[__cond, __body] : ctx->branches) {
        auto *__branch_body = IRpool::allocate <block> ();
        auto *__branch_fail = IRpool::allocate <block> ();

        branch_stack.push_back({__branch_fail, __branch_body});
        visit(__cond);
        end_block(get_branch());
        branch_stack.pop_back();

        add_block(__branch_body);
        visit(__body);
        end_block(IRpool::allocate <jump_stmt> (__branch_exit));

        add_block(__branch_fail);
    }

    if (ctx->else_body) visit(ctx->else_body);

    end_block(IRpool::allocate <jump_stmt> (__branch_exit));
    add_block(__branch_exit);
}

void IRbuilder::visitVariableDef(AST::variable_def *ctx) {
    if (top == nullptr) { // Global variable.
        for (auto &&[__name, __init] : ctx->vars) {
            visitGlobalVariable(
                safe_cast <AST::variable *> (global_scope->find(__name)),
                dynamic_cast <AST::literal_expr *> (__init));
        }
    } else { // Local variable.
        for (auto &&[__name, __init] : ctx->vars) {
            if (!__init) continue; // Nothing to do :)
            visit(__init);
            /* The local variable_map has been built in create_function */
            top_block->push_back(IRpool::allocate <store_stmt> (
                get_value(), variable_map[ctx->field->find(__name)]));
        }
    }
}

void IRbuilder::visitFunctionDef(AST::function_def *ctx) {
    top         = function_map[ctx];    // given function
    top_block   = top->data.front();    // entry, .bb0
    runtime_assert(top->data.size() == 1 && branch_stack.empty(), "wtf?");

    set_invalid();
    visit(ctx->body);

    runtime_assert(branch_stack.empty(), "wtf?");

    /* Check for the missing return. */
    bool __tag {};
    if (top != main_function) {
        if (top->type == typeinfo { void_type::ptr(), 0 }) {
            for (auto *__block : top->data) {
                if (!__block->flow)
                    __block->flow = IRpool::allocate <return_stmt> (nullptr, top);
            }
        } else {
            /* Warning of no-return value. */
            for (auto *__block : top->data) {
                if (!__block->flow) {
                    __tag = true;
                    __block->flow = IRpool::allocate <unreachable_stmt> ();
                }
            }
            if (__tag) warning(std::format(
                "Missing return value in function \"{}\"", top->name));
        }
    } else { /* Add back return 0. */
        for (auto *__block : top->data) {
            if (!__block->flow) {
                __tag = true;
                __block->flow = IRpool::allocate <return_stmt> (IRpool::__zero__, top);
            }
        }
        if (__tag) warning(
            "Missing return value in main function.\t"
            "\"return 0;\" is added automatically."
        );
    }

}

void IRbuilder::visitClassDef(AST::class_def *ctx) {
    for (auto *__p : ctx->member)
        if (auto *__func = dynamic_cast <AST::function_def *> (__p))
            return visitFunctionDef(__func);
}

void IRbuilder::visitGlobalVariable(AST::variable *ctx, AST::literal_expr *__lit) {
    auto *__var = &global_variables.emplace_back();
    __var->type = ++transform_type(ctx->type);
    __var->name = ctx->unique_name;
    variable_map[ctx] = __var;

    IR::literal *__ret;
    auto __name = (--__var->type).name();
    if (__name == "i1") {
        __ret = IRpool::create_boolean(__lit && __lit->name == "true");
    } else if (__name == "i32") {
        __ret = IRpool::create_integer(__lit ? std::stoi(__lit->name) : 0);
    } else if (__lit && __lit->sort == __lit->STRING) {
        __ret = IRpool::create_pointer(
            IRpool::create_string(Mx_string_parse(__lit->name)));
    } else { // Null pointer case
        __ret = IRpool::__null__;
    }

    __var->init = __ret;
    __var->is_constant = false; // Of course, we can't tell it now.
}

/**
 * @brief Creating an array recursively.
 * @param __type Type of the array.
 * @param __args Index of each dimension (reverse order).
 */
void IRbuilder::visitNewArray(typeinfo __type, std::vector <definition *> __args) {
    throw std::string("Debug");
    runtime_assert(false, "Not implemented yet.");
}

} // namespace dark::IR
