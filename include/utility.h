#pragma once
#include <iostream>
#include <string>
#include <format>
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
inline void runtime_assert(bool __cond, std::string_view __msg) {
    if (__builtin_expect(__cond, true)) return;
    throw error(std::string(__msg));
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

} // namespace dark
