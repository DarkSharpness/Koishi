#pragma once
#include "LOOP/loop.h"
#include <unordered_map>

namespace dark::IR {

struct LoopNestDetector {
    LoopNestDetector(function *);
    static bool clean(function *);  // Clear all previous info.
  private:
    static void setProperty(function *);
    static bool checkProperty(function *);
};


} // namespace dark::IR
