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

/* Cast to derived may throw! */
template <typename _Up, typename _Tp>
requires std::is_base_of_v <_Tp, _Up>
inline _Up *safe_cast(_Tp *__val) {
    return &dynamic_cast <_Up &> (*__val);
}
/* Cast to base is safe. */
template <typename _Up, typename _Tp>
requires std::is_base_of_v <_Up, _Tp>
inline _Up *safe_cast(_Tp *__val) noexcept {
    return static_cast <_Up *> (__val);
}


} // namespace dark
