#pragma once
#include "utility.h"

/* IR type related part. */
namespace dark::IR {

struct function;
inline static constexpr std::size_t kMxPtrSize = 4;

struct class_type {
    virtual bool is_trivial()   const = 0;
    virtual std::size_t size()  const = 0;
    virtual std::string_view name()  const = 0;
    virtual ~class_type() = default;
};

struct typeinfo {
    using ssize_t = std::make_signed_t <std::size_t>;

    const class_type* base;     // Base type.
    ssize_t     dimensions {};  // Pointer dimensions.

    std::string_view name() const {
        if (dimensions > 0) return "ptr";
        else                return base->name();
    }
    std::size_t size() const { return dimensions > 0 ? kMxPtrSize : base->size(); }

    typeinfo operator ++(void) const { return { base, dimensions + 1 }; }
    typeinfo operator --(void) const { return (*this) + (-1); }
    typeinfo operator -(ssize_t __n) const { return (*this) + (-__n); }
    typeinfo operator +(ssize_t __n) const {
        runtime_assert(dimensions + __n >= 0, "No dereference for non-pointer type");
        return { base, dimensions + __n };
    }

};

/* Void type. */
struct void_type final : class_type {
  private:
    void_type() noexcept = default;
  public:
    static void_type *ptr() { static void_type __type; return &__type; }
    bool is_trivial()   const override { return true; }
    std::size_t size()  const override {
        runtime_assert(false, "Cannot evaluate size of void type");
        __builtin_unreachable();
    }
    std::string_view name()  const override { return "void"; }
};

/* Integer type. */
struct int_type final : class_type {
  private:
    int_type() noexcept = default;
  public:
    static int_type *ptr() { static int_type __type; return &__type; }
    bool is_trivial()   const override { return true; }
    std::size_t size()  const override { return 4; }
    std::string_view name()  const override { return "int"; }
};

/* Boolean type. */
struct bool_type final : class_type {
  private:
    bool_type() noexcept = default;
  public:
    static bool_type *ptr() { static bool_type __type; return &__type; }
    bool is_trivial()   const override { return true; }
    std::size_t size()  const override { return 1; }
    std::string_view name()  const override { return "bool"; }
};


/* Char array type. */
struct cstring_type final : class_type {
  private:
    std::size_t length;
    std::string _name_;
  public:
    cstring_type(std::size_t __len) : length(__len) {
        _name_ = std::format("[{} x i8]", length + 1);
    }
    bool is_trivial()   const override { return true; }
    std::size_t size()  const override { return length; }
    std::string_view name()  const override { return _name_; }
};

/* String type. */
struct string_type final : class_type {
  private:
    string_type() noexcept = default;
  public:
    static string_type *ptr() { static string_type __type; return &__type; }
    bool is_trivial()   const override { return false; }
    std::size_t size()  const override { return kMxPtrSize; }
    std::string_view name()  const override { return "string"; }
};

/* Null type. */
struct null_type final : class_type {
  private:
    null_type() noexcept = default;
  public:
    static null_type *ptr() { static null_type __type; return &__type; }
    bool is_trivial()   const override { return true; }
    std::size_t size()  const override { return kMxPtrSize; }
    std::string_view name()  const override { return "null"; }
};

/* User-defined classes. */
struct custom_type final : class_type {
  public:
    function *  constructor{};
    std::string class_name;   // '%' + class name

    std::vector <typeinfo>      layout;
    std::vector <std::string>   member;

    custom_type(std::string __name) : class_name(std::move(__name)) {}

    /* Return the index of a certain member. */
    std::size_t get_index(std::string_view __name) const {
        for (std::size_t i = 0; i < member.size(); ++i)
            if (member[i] == __name) return i;
        runtime_assert(false, "No such member");
        __builtin_unreachable();
    }

    bool is_trivial()   const override { return false; }
    std::size_t size()  const override { return layout.size() * kMxPtrSize; }
    std::string_view name()  const override { return class_name; }
    std::string data()  const;  // Data to print.
};

inline bool __is_null_type(const typeinfo &__type) {
    return __type.dimensions == 0 && __type.base == null_type::ptr();
}
inline bool __is_int_type(const typeinfo &__type) {
    return __type.dimensions == 0 && __type.base == int_type::ptr();
}
inline bool __is_bool_type(const typeinfo &__type) {
    return __type.dimensions == 0 && __type.base == bool_type::ptr();
}

inline bool operator == (const typeinfo &lhs, const typeinfo &rhs) {
    if (lhs.dimensions == rhs.dimensions) {
        return lhs.base == rhs.base;
    } else { // At least one of them must be null type.
        return __is_null_type(lhs) || __is_null_type(rhs);
    }
}



} // namespace dark::IR
