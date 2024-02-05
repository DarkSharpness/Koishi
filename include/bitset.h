#pragma once
#include <bit>          // std::popcount
#include <vector>       // std::vector
#include <string>       // std::string_view
#include <cstring>      // std::memset
#include <climits>      // CHAR_BIT
#include <cstdint>      // std::uint64_t

namespace dark {

struct dynamic_bitset {
  public:
    using _Bitset = dynamic_bitset;
    struct reference {
        _Bitset *   ptr;
        std::size_t pos;
        bool operator = (bool val) { ptr->set(pos, val); return val; }
        operator bool() const { return ptr->test(pos); }
    };

  private:
    using ll = std::uint64_t;
    inline static constexpr ll Bits = sizeof(ll) * CHAR_BIT;

    friend class reference;

    std::vector <ll> data   {};
    std::size_t length      {};

    /* Mask first n low bits to 1 */
    static ll mask_low (std::size_t n) { return (ll(1) << n) - 1; }
    /* Mask higher-than-n bits to 1 */
    static ll mask_high(std::size_t n) { return ll(-1) << n ; }
    /* Round up n / 64 */
    static ll round_up(std::size_t n) { return (n + Bits - 1) / Bits; }
    /* Make valid thte bitset */
    dynamic_bitset &validate() {
        const std::size_t mod = length % Bits;
        if (mod) data.back() &= mask_low(mod);
        return *this;
    }

  public:
    inline static constexpr std::size_t npos = -1;

    dynamic_bitset() noexcept = default;
    ~dynamic_bitset() noexcept = default;

    dynamic_bitset(const dynamic_bitset &) = default;
    dynamic_bitset &operator = (const dynamic_bitset &) = default;

    dynamic_bitset(dynamic_bitset &&) noexcept = default;
    dynamic_bitset &operator = (dynamic_bitset &&) noexcept = default;

    explicit dynamic_bitset(std::size_t n) : data(round_up(n)), length(n) {}

    explicit dynamic_bitset(std::string_view str) : dynamic_bitset(str.size()) {
        for (std::size_t i = 0 ; i != str.size() ; ++i)
            data[i / Bits] |= ll(str[i] - '0') << (i % Bits);
    }

    bool none() const {
        for (auto __word : data) if (__word != 0) return false;
        return true;
    }
    bool all() const {
        if (data.empty()) return true;
        for (std::size_t i = 0 ; i != data.size() - 1 ; ++i)
            if (data[i] != ll(-1)) return false;
        const std::size_t mod = length % Bits;
        return mod ? (data.back() == mask_low(mod)) : (data.back() == ll(-1));
    }
    bool any() const { return !none(); }

    std::size_t size() const { return length; }

    _Bitset &operator |= (const _Bitset &rhs) {
        const std::size_t len = std::min(length, rhs.length);
        const std::size_t div = len / Bits;
        const std::size_t mod = len % Bits;
        for (std::size_t i = 0 ; i != div ; ++i) data[i] |= rhs.data[i];
        if (mod) data[div] |= rhs.data[div] & mask_low(mod);
        return *this;
    }
    _Bitset &operator &= (const _Bitset &rhs) {
        const std::size_t len = std::min(length, rhs.length);
        const std::size_t div = len / Bits;
        const std::size_t mod = len % Bits;
        for (std::size_t i = 0 ; i != div ; ++i) data[i] &= rhs.data[i];
        if (mod) data[div] &= rhs.data[div] | mask_high(mod);
        return *this;
    }
    _Bitset &operator ^= (const _Bitset &rhs) {
        const std::size_t len = std::min(length, rhs.length);
        const std::size_t div = len / Bits;
        const std::size_t mod = len % Bits;
        for (std::size_t i = 0 ; i != div ; ++i) data[i] ^= rhs.data[i];
        if (mod) data[div] ^= rhs.data[div] & mask_low(mod);
        return *this;
    }
    _Bitset &operator <<= (std::size_t n) {
        length += n;
        const std::size_t div = n / Bits;
        const std::size_t mod = n % Bits;
        data.insert(data.begin(), div, 0);
        if (mod) {
            ll  carry   = 0;
            for (std::size_t i = div ; i != data.size() ; ++i) {
                ll &dat = data[i];
                ll  tmp = dat >> (Bits - mod);
                dat     = (dat << mod) | carry;
                carry   = tmp;
            }
            if (round_up(length) != data.size()) data.push_back(carry);
        } return *this;
    }
    _Bitset &operator >>= (std::size_t n) {
        if (n >= length) { length = 0; data.clear(); return *this; }
        length -= n;
        const std::size_t div = n / Bits;
        const std::size_t mod = n % Bits;
        data.erase(data.begin(), data.begin() + div);
        if (mod) {
            ll  carry   = 0;
            for (std::size_t i = data.size() ; i != 0 ; --i) {
                ll &dat = data[i - 1];
                ll  tmp = dat << (Bits - mod);
                dat     = (dat >> mod) | carry;
                carry   = tmp;
            }
            if (round_up(length) != data.size()) data.pop_back();
        } return *this;
    }

    _Bitset &set() {
        std::memset(data.data(), -1, data.size() * sizeof(ll));
        return validate();
    }

    _Bitset &flip() {
        for (auto &__word : data) __word = ~__word;
        return validate();
    }

    _Bitset &reset() {
        std::memset(data.data(), 0, data.size() * sizeof(ll));
        return *this;
    }

    void set(std::size_t n, bool val = true) {
        const std::size_t div = n / Bits;
        const std::size_t mod = n % Bits;
        if (val) data[div] |=   ll(1) << mod;
        else     data[div] &= ~(ll(1) << mod);
    }
    bool test(std::size_t n) const {
        const std::size_t div = n / Bits;
        const std::size_t mod = n % Bits;
        return (data[div] >> mod) & 1;
    }

    void push_back(bool val) {
        if (length % Bits == 0) data.push_back(val);
        else data.back() |= ll(val) << (length % Bits);
        ++length;
    }
    void pop_back(bool val) {
        if (length % Bits == 0) data.pop_back();
        else data.back() &= ~(ll(1) << (length % Bits));
        --length;
    }
    void resize(std::size_t n) {
        data.resize(round_up(n));
        length = n;
        if (n < length) validate();
    }

    void clear() { data.clear(); length = 0; }

    _Bitset operator ~() const {
        auto ret = *this; ret.flip(); return ret;
    }
    friend _Bitset operator | (const _Bitset &lhs, const _Bitset &rhs) {
        auto ret = lhs; ret |= rhs; return ret;
    }
    friend _Bitset operator & (const _Bitset &lhs, const _Bitset &rhs) {
        auto ret = lhs; ret &= rhs; return ret;
    }
    friend _Bitset operator ^ (const _Bitset &lhs, const _Bitset &rhs) {
        auto ret = lhs; ret ^= rhs; return ret;
    }
    friend _Bitset operator << (const _Bitset &lhs, std::size_t n) {
        auto ret = lhs; ret <<= n; return ret;
    }
    friend _Bitset operator >> (const _Bitset &lhs, std::size_t n) {
        auto ret = lhs; ret >>= n; return ret;
    }

    friend bool operator == (const _Bitset &lhs, const _Bitset &rhs) {
        if (lhs.length != rhs.length) return false;
        return !std::memcmp(
            lhs.data.data(), rhs.data.data(), lhs.data.size() * sizeof(ll));
    }

    bool operator [] (std::size_t n) const { return test(n); }
    reference operator [] (std::size_t n) { return reference(this, n); }

    std::size_t count() const {
        std::size_t ret = 0;
        for (auto __word : data) ret += std::popcount(__word);
        return ret;
    }

    std::size_t find_from(std::size_t __n) {
        if (__n >= length) return npos;
        const std::size_t div = __n / Bits;
        const std::size_t mod = __n % Bits;
        if (auto __word = data[div] >> mod)
            return div * Bits + std::countr_zero(__word) + mod;
        for (std::size_t i = div + 1 ; i < data.size() ; ++i)
            if (data[i]) return i * Bits + std::countr_zero(data[i]);
        return npos;
    }

    std::size_t find_first() {
        for (std::size_t i = 0; i < data.size() ; ++i)
            if (data[i]) return i * Bits + std::countr_zero(data[i]);
        return npos;    
    }
};

} // namespace dark
