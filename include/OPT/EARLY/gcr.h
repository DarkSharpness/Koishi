#pragma once
#include "IRbase.h"
#include <unordered_set>

namespace dark::IR {

struct GlobalConstantReplacer {
    GlobalConstantReplacer(IRbuilder *);
  private:
    std::unordered_set <global_variable *> useSet;
    std::unordered_map <temporary *,literal *> valMap;
    void scan(function *);
    void scanInit(function *);
    void replace(function *);
};

} // namespace dark::IR
