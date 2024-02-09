#pragma once
#include "IRbase.h"


namespace dark::IR {

struct loopInfo {
    loopInfo *parent {};
    block    *header {};
    std::vector <block *> body;
    std::size_t depth {};
};

} // namespace dark::IR
