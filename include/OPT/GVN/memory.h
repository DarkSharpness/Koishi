#pragma once
#include "IRbase.h"
#include <unordered_set>

namespace dark::IR::__gvn {

struct memorySimplifier {
    struct memInfo {
        definition *val     {};
        store_stmt *last    {};
    };

    using _Mmap_t = std::unordered_map <definition *, memInfo>;
    using _Mset_t = std::unordered_set <store_stmt *>;

    _Mmap_t memMap;     // Memory info map.
    _Mset_t deadStore;  // Dead store recorder.

    void analyzeStore(store_stmt *);
    definition *analyzeCall(call_stmt *);
    definition *analyzeLoad(load_stmt *);
    void reset();
};


} // namespace dark::IR
