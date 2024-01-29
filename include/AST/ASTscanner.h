// This file should be included only once.
// Some no pragma once here.
// #pragma once
#include "ASTnode.h"
#include "ASTscope.h"
#include "ASTbuilder.h"
#include <ranges>

namespace dark::AST {

struct scanner {
  protected:
    using _Map_t        = class_list;
    using _Alloc_Node   = central_allocator <node>; 
    using _Alloc_Scope  = central_allocator <scope>;

    _Map_t          &class_map;
    _Alloc_Node     &node_alloc;
    _Alloc_Scope    &scope_alloc;

    function *create_function(typeinfo __type, const char *__name,
        const char *__unique_name) {
        auto *__func = node_alloc.allocate <function> ();
        __func->type = __type;
        __func->name = __name;
        __func->unique_name = __unique_name;
        return __func;
    }
    
    function *create_function(typeinfo __type, const char *__name,
        const char *__unique_name, argument_list __args) {
        auto *__func = create_function(__type, __name, __unique_name);
        __func->args = std::move(__args);
        return __func;
    }

    variable *create_variable(typeinfo __type, std::string __name,
        std::string __unique_name) {
        auto *__var = node_alloc.allocate <variable> ();
        __var->type = __type;
        __var->name = std::move(__name);
        __var->unique_name = std::move(__unique_name);
        return __var;
    }

    scanner(ASTbuilder *__builder, _Alloc_Scope *__alloc)
    : class_map(__builder->classes), node_alloc(__builder->pool), scope_alloc(*__alloc) {}
};

} // namespace dark::AST

namespace dark::AST {

struct class_scanner : scanner {
  private:
    definition_list &global;    // Global definitions.

    static class_def *to_class(definition *__def) {
        return dynamic_cast <class_def *> (__def);
    }
    static bool non_null(class_def *__def) {
        return __def != nullptr;
    }

    /* Insert all builtin types. */
    void insert_builtin() {
        class_map.emplace("int");
        class_map.emplace("bool");
        class_map.emplace("void");
        class_map.emplace("null");
        class_map.emplace("string");
        class_map.emplace("_Array");
    }

    /* Insert some builtin member functions for builtin types. */
    void insert_member() {
        typeinfo __int_type = { &class_map["int"], 0, 0 };
        typeinfo __str_type = { &class_map["string"], 0, 0 };

        do {    /* _Array type */
            auto *__temp = scope_alloc.allocate();
            class_map["_Array"].field = __temp;
            /* Insert builtin member functions. */
            __temp->insert(create_function(__int_type, "size", "_Array::size"));
        } while(false);

        do {    /* String type. */
            auto *__temp = scope_alloc.allocate();
            class_map["string"].field = __temp;
            /* Insert builtin member functions. */
            __temp->insert(create_function(__int_type, "length", "string::length"));
            __temp->insert(create_function(__int_type, "parseInt", "string::parseInt"));
            __temp->insert(create_function(__str_type, "substring", "string::substring",
                {{ __int_type, "l" }, { __int_type, "r" }}));
            __temp->insert(create_function(__int_type, "ord", "string::ord",
                {{ __int_type, "pos" }}));
        } while(false);
    }

    /* Check the existence of all typeinfo used. */
    void check_typeinfo() {
        using std::views::transform;
        using std::views::filter;
        /* Set of all defined classes. */
        std::unordered_set <std::string_view> name_set = {
            "int", "bool", "void", "null", "string", "_Array"
        };

        /* Take record of all classes defined. */
        for (auto *__class : global | transform(to_class) | filter(non_null)) {
            runtime_assert(name_set.insert(__class->name).second,
                "Duplicated class name: ", __class->name);
            class_map[__class->name].field = __class->field = scope_alloc.allocate();
        }

        /* Check whether all used types are defined. */
        std::string __msg;
        for (auto &__class : class_map)
            if (!name_set.count(__class.name))
                __msg += '\"' + __class.name + "\" ";

        runtime_assert(__msg.empty(), "Undefined class name: ", __msg);
    }

  public:

    class_scanner(ASTbuilder *builder, _Alloc_Scope *alloc)
    : scanner(builder, alloc), global(builder->global) {
        this->insert_builtin();
        this->insert_member();
        this->check_typeinfo();
    }
};

} // namespace dark::AST



namespace dark::AST {

struct function_scanner : scanner {
  public:
    scope *global;          // Global scope.
  private:
    class_type *void_class {};  // Special typeinfo for void pointer.

    /* Insert builtin global functions. */
    void insert_global() {
        typeinfo __int_type = { &class_map["int"], 0, 0 };
        typeinfo __str_type = { &class_map["string"], 0, 0 };
        typeinfo __nil_type = { &class_map["void"], 0, 0 };
        global->insert(create_function(__nil_type, "print", "::print", {{ __str_type, "str" }}));
        global->insert(create_function(__nil_type, "println", "::println", {{ __str_type, "str" }}));
        global->insert(create_function(__nil_type, "printInt", "::printInt", {{ __int_type, "n" }}));
        global->insert(create_function(__nil_type, "printlnInt", "::printlnInt", {{ __int_type, "n" }}));
        global->insert(create_function(__str_type, "getString", "::getString"));
        global->insert(create_function(__int_type, "getInt", "::getInt"));
        global->insert(create_function(__str_type, "toString", "::toString", {{ __int_type, "n" }}));
    }

    /* Check whether there are invalid void in the function. */
    void check_void(function *__func) {
        if (!void_class) void_class = &class_map["void"];

        if (__func->type.base == void_class)
            runtime_assert(__func->type.dimensions == 0, "Void cannot be array");

        for (auto &__arg : __func->args)
            runtime_assert(__arg.type.base != void_class, "Void cannot be argument type");
    }

    /* Check for global functions. */
    void check_global(function *__func) {
        check_void(__func);
        runtime_assert(!class_map.count(__func->name),
            "Function name cannot be class name: \"", __func->name, "\"");
        runtime_assert(global->insert(__func),
            "Duplicated function name: \"", __func->name, "\"");
    }

    /* Check for member functions. */
    void check_member(function *__func, scope *__field) {
        check_void(__func);
        if (!__field->insert(__func)) {
            if (__func->name == "")
                runtime_assert(false,
                    "Constructor cannot be overloaded: \"" , __func->type.data(), "\"");
            else // Non-constructor function.
                runtime_assert(false,
                    "Duplicated member function name: \"", __func->name, "\"");
            __builtin_unreachable();
        }
    }

    /* Check functions. */
    void check_function(definition_list &list) {
        for (auto *__def : list) {
            if (auto __func = dynamic_cast <function *> (__def)) {
                check_global(__func);
                /* Set up function scope. */
                __func->field = scope_alloc.allocate();
                __func->field->prev = global;
                __func->unique_name = "::" + __func->name;
            } else if (auto __class = dynamic_cast <class_def *> (__def)) {
                check_class(__class);
                /* Set up the class scope. */
                __class->field->prev = global;
            }
        }
    }

    /* Check class content. */
    void check_class(class_def *__class) {
        for (auto __node : __class->member) {
            if (auto __func = dynamic_cast <function_def *> (__node)) {
                check_member(__func, __class->field);

                runtime_assert(__func->name != __class->name,
                    "Function name cannot be class name");

                runtime_assert(__func->name.size() || __func->type.data() == __class->name,
                    "Constructor name must be the same as class name");

                __func->field = scope_alloc.allocate();
                __func->field->prev = __class->field;
                __func->unique_name = __class->name + "::" + __func->name;

                /* Insert this pointer. ("this" is not assignable) */
                __func->field->insert(create_variable(
                    { &class_map[__func->type.data()], 0, false },
                    "this", __func->unique_name + "::this"
                ));
            } else { // Member variable.
                auto *__list = safe_cast <variable_def *> (__node);
                for (auto &&[__name, __init] : __list->vars) {
                    runtime_assert(__init == nullptr,
                        "Member variable cannot have default value");

                    /* Insert and check duplicate of member variables. */
                    runtime_assert(__class->field->insert(
                        create_variable(__list->type, __name, __class->name + "::" + __name)
                    ), "Duplicated member variable name: ", __name);
                }
            }
        }
    }

  public:
    function_scanner(ASTbuilder *builder, _Alloc_Scope *alloc)
    : scanner(builder, alloc), global(alloc->allocate()) {
        this->insert_global();
        this->check_function(builder->global);
    }
};

} // namespace dark::AST

namespace dark::AST {

/* Checking operation on the same type. */
struct type_checker {
    static bool is_cmp(const operand_t &__op) {
        return __op.str[1] == '='   // != , == , <= , >=
           || (__op.str[1] == 0 && (__op.str[0] == '<' || __op.str[0] == '>'));
    }

    static const char *check_int(binary_expr *ctx) {
        if (ctx->op.str[0] == ctx->op.str[1])
            runtime_assert(ctx->op.str[0] != '&' && ctx->op.str[0] != '|',
                "Invalid logical operator on int");
        return is_cmp(ctx->op) ? "bool" : "int";
    }

    static const char *check_bool(binary_expr *ctx) {
        switch (ctx->op.str[0]) {
            case '&': case '|': // && and || are valid.
                if (ctx->op.str[1]) break;
            case '>': case '<': case '+': case '-': case '*': case '/': case '%': case '^':
                runtime_assert(false, "Invalid operator on bool");
        } return "bool";
    }

    static const char *check_string(binary_expr *ctx) {
        if (is_cmp(ctx->op)) return "bool";
        if (ctx->op == "+") return "string";
        runtime_assert(false, "Invalid operator on string");
        __builtin_unreachable();
    }

    /* For class/null type, only != and == are allowed. */
    static const char *check_other(binary_expr *ctx) {
        if (ctx->op.str[0] == '=' || ctx->op.str[0] == '!') return "bool";
        runtime_assert(false, "Invalid operator on ", ctx->lval->type.data());
        __builtin_unreachable();
    }
};

} // namespace dark::AST
