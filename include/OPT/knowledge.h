#pragma once
#include "IRbase.h"
#include <cstdint>
#include <bitset>

namespace dark::IR {

struct sizeInfo {
    int32_t lower = INT32_MIN;  // Minimum possible value.
    int32_t upper = INT32_MAX;  // Maximum possible value.
    sizeInfo() = default;
    explicit sizeInfo(int __val) { upper = lower = __val; }
    explicit sizeInfo(int64_t __low, int64_t __top) {
        if (__low < INT32_MIN) __low = INT32_MIN;
        if (__top > INT32_MAX) __top = INT32_MAX;
        lower = __low; upper = __top;
        runtime_assert(upper >= lower, "Invalid sizeInfo");
    }
    friend bool operator <= (sizeInfo,sizeInfo);
};

struct bitsInfo {
    int32_t state;  // 0 = false    1 = true
    int32_t valid;  // 0 = invalid  1 = certain
    bitsInfo() = default;
    explicit bitsInfo(int __val) : state(__val), valid(-1) {}
    explicit bitsInfo(int32_t __state, int32_t __valid)
        : state(__state), valid(__valid) {}
    friend bool operator <= (bitsInfo,bitsInfo);
};

struct knowledge {
    sizeInfo size;  // Range information.
    bitsInfo bits;  // Bits information.

    void init(sizeInfo);
    void init(bitsInfo);
};


} // namespace dark::IR
