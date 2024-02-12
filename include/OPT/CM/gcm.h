#pragma once
#include "IRbase.h"
#include <unordered_set>

namespace dark::IR {


struct GlobalCodeMotionPass {
    GlobalCodeMotionPass(function *);

  private:
    std::unordered_set <node *> visited;
    struct defInfo {
        block *attached; // Final block attached.
        std::vector <node *> useList;
    };
    std::unordered_map <node *, defInfo> defMap;
    block *__entry; // Special entry block.
    std::unordered_map <block *, std::vector <node *>> newList;

    void initBlock(block *);
    void initUseList(block *);

    void scheduleEarly(statement *);
    void scheduleLate(statement *);
    void scheduleBlock(statement *,block *, block *);

    bool checkProperty(function *);
    void setProperty(function *);

    void prepareMotion(block *);
    void arrangeMotion(block *);

    void motionDfs(statement *, block *);
};


} // namespace dark::IR
