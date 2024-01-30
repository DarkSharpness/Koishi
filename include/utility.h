#pragma once
#include <string>
#include <format>
#include <vector>
#include <iostream>
#include <concepts>

namespace dark {

struct error : std::exception {
    std::string msg;
    error(std::string __s) : msg(std::move(__s)) {
        std::cerr << std::format("\033[31mError: {}\n\033[0m", msg);
    }
    const char *what() const noexcept override { return msg.c_str(); }
};

struct warning : std::exception {
    std::string msg;
    warning(std::string __s) : msg(std::move(__s)) {
        std::cerr << std::format("\033[33mWarning: {}\n\033[0m", msg);
    }
    const char *what() const noexcept override { return msg.c_str(); }
};

/* Check in runtime. If fails, throw. */
template <typename ..._Args>
inline void runtime_assert(bool __cond, _Args &&...__args) {
    if (__builtin_expect(__cond, true)) return;
    throw error((std::string(std::forward <_Args>(__args)) + ...));
}

/* Cast to base is safe. */
template <typename _Up, typename _Tp>
requires std::is_base_of_v <std::remove_pointer_t<_Up>, _Tp>
inline _Up safe_cast(_Tp *__ptr) noexcept {
    return static_cast <_Up> (__ptr);
}

/* Cast to derived may throw! */
template <typename _Up, typename _Tp>
requires (!std::is_base_of_v <std::remove_pointer_t<_Up>, _Tp>)
inline _Up safe_cast(_Tp *__ptr) {
    auto __tmp = dynamic_cast <_Up> (__ptr);
    runtime_assert(__tmp, "Cast failed.");
    return __tmp;
}


namespace detail {

template <typename _Tp>
concept __value_hidable = (sizeof(_Tp) <= sizeof(void *)) && std::is_trivial_v <_Tp>;

} // namespace detail

/* A wrapper that is using to help hide implement. */
struct hidden_impl {
  private:
    void *impl {};
  public:
    /* Set the hidden implement pointer. */
    void set_impl_ptr(void *__impl) noexcept { impl = __impl; }
    /* Set the hidden implement value. */
    template <class T> requires detail::__value_hidable <T>
    void set_impl_val(T __val) noexcept { get_impl_val <T> () = __val; }

    /* Get the hidden implement pointer. */
    template <class T = void>
    T *get_impl_ptr() const noexcept { return static_cast <T *> (impl); }
    /* Get the hidden implement value. */
    template <class T> requires detail::__value_hidable <T>
    T &get_impl_val() noexcept { return *(reinterpret_cast <T *> (&impl)); }
};

/* A central allocator that is intented to avoid memleak. */
template <typename _Vp>
struct central_allocator {
 private:
    std::vector <_Vp *> data;
  public:
    /* Allocate one node. */
    template <typename _Tp = _Vp, typename ..._Args>
    requires std::is_base_of_v <_Vp, _Tp>
    _Tp *allocate(_Args &&...args) {
        auto *__ptr = new _Tp(std::forward <_Args>(args)...);
        return data.push_back(__ptr), __ptr;
    }

    ~central_allocator() { for (auto *__ptr : data) delete __ptr; }
};

template <typename _Range>
std::string join_strings(_Range &&__container) {
    std::size_t __cnt {};
    std::string __ret {};
    for (auto &__str : __container) __cnt += __str.size();
    __ret.reserve(__cnt);
    for (auto &__str : __container) __ret += __str;
    return __ret;
}

/* Parsing AST string input into real ASCII string. */
inline std::string Mx_string_parse(std::string_view __src) {
    std::string __dst;
    if(__src.front() != '\"' || __src.back() != '\"')
        runtime_assert(false, "Invalid string literal.");
    __src.remove_prefix(1);
    for(size_t i = 1 ; i < __src.length() ; ++i) {
        if(__src[i] == '\\') {
            switch(__src[++i]) {
                case 'n':  __dst.push_back('\n'); break;
                case '\"': __dst.push_back('\"'); break;
                case '\\': __dst.push_back('\\'); break;
                default: throw error("Invalid escape sequence.");
            }
        } else __dst.push_back(__src[i]);
    } return __dst;
}

/* Fix-sized vector. */
template <typename _Tp, std::size_t _Nm>
struct fixed_vector {
  protected:
    _Tp         data[_Nm];
    std::size_t length {};
  public:
    using value_type        = _Tp;
    using size_type         = std::size_t;
    using difference_type   = std::ptrdiff_t;
    using reference         = _Tp &;
    using const_reference   = const _Tp &;
    using pointer           = _Tp *;
    using const_pointer     = const _Tp *;
    using iterator          = _Tp *;
    using const_iterator    = const _Tp *;

    std::size_t size() const noexcept { return length; }

    void push_back(const _Tp &__val) {
        runtime_assert(length < _Nm, "Vector overflow.");
        data[length++] = __val;
    }
    void pop_back() noexcept { --length; }
    void clear()    noexcept { length = 0; }
    void resize(std::size_t __sz) {
        runtime_assert(__sz <= _Nm, "Vector overflow.");
        length = __sz;
    }

    reference front() noexcept { return data[0]; }
    reference back()  noexcept { return data[length - 1]; }
    reference operator[](std::size_t __pos) noexcept { return data[__pos]; }

    const_reference front()  const noexcept { return data[0]; }
    const_reference back()   const noexcept { return data[length - 1]; }
    const_reference operator[](std::size_t __pos) const noexcept { return data[__pos]; }

    iterator begin() noexcept { return data; }
    iterator end()   noexcept { return data + length; }
    const_iterator begin()  const noexcept { return data; }
    const_iterator end()    const noexcept { return data + length; }
    const_iterator cbegin() const noexcept { return data; }
    const_iterator cend()   const noexcept { return data + length; }
};


} // namespace dark
