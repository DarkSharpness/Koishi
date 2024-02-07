#pragma once
#include <version>

namespace dark {

/* Fix-sized vector. */
template <typename _Tp, std::size_t _Nm>
struct fixed_vector {
  protected:
    _Tp         data[_Nm]   {};
    std::size_t length      {};
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

    fixed_vector() noexcept = default;
    fixed_vector(std::initializer_list <_Tp> __list) {
        runtime_assert(__list.size() <= _Nm, "Vector overflow.");
        for (auto &__val : __list) data[length++] = __val;
    }

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
