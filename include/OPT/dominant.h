#pragma once
#include "IRbase.h"
#include <unordered_set>

namespace dark::IR {

struct dominantMaker {
  protected:
    inline static block dummy {};
    void makeRpo(block *);
    void initEdge(function *);
    void iterate(block *);
    bool update(block *);
    void removeDummy(function *);

  public:
    struct _Info_t {
        std::vector <block *> dom;  // Dominator
        std::vector <block *> fro;  // Dominance frontier
    };

    std::vector         <block *> rpo;
    std::unordered_set  <block *> visited;

    dominantMaker(function *, bool = false);
    void clean(function *);

};

inline std::vector <block *> &getDom(block *__blk) {
    using _Info_t = dominantMaker::_Info_t;
    return __blk->get_impl_ptr <_Info_t> ()->dom;
}

inline std::vector <block *> &getFrontier(block *__blk) {
    using _Info_t = dominantMaker::_Info_t;
    return __blk->get_impl_ptr <_Info_t> ()->fro;
}


} // namespace dark::IR
