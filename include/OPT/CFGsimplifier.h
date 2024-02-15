#pragma once
#include "IRbase.h"
#include <unordered_set>

namespace dark::IR {

struct CFGsimplifier {
    CFGsimplifier(function *);
    /* This is used to eliminate single phi. */
    std::unordered_map <block *, block *> remap; 
    void dfs(block *);
    void update(block *);
    void remove(function *);
};

} // namespace dark::IR
