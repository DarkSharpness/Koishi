#pragma once
#include "IRbase.h"
#include <unordered_set>

namespace dark::IR {

struct dominantMaker {
  protected:
    inline static constexpr block &dummy = IRpool::__dummy__;
    static void initEdge(function *);
    void makePostOrder(block *);
    void iterate(block *);
    bool update(block *);
    void buildFrontier();
    void removeDummy();

  public:
    struct _Info_t {
        std::vector <block *> dom;  // Dominator
        std::vector <block *> fro;  // Dominance frontier
        void set_idom(block *__node) {  get_storage()[0] = __node;  }
        block *get_idom()            {  return get_storage()[0];    }
        void set_fail(block *__node) {  get_storage()[1] = __node;  }
        block *get_fail()            {  return get_storage()[1];    }
      private:
        block **get_storage() { return reinterpret_cast <block **> (&fro); }
    };

    std::vector         <block *> rpo;
    std::unordered_set  <block *> visited;

    dominantMaker(function *, bool = false);
    static void clean(function *);
};

inline auto &getDomInfo(block *__blk) {
    using _Info_t = dominantMaker::_Info_t;
    return *__blk->get_ptr <_Info_t> ();
}
inline std::vector <block *> &getDomSet(block *__blk) {
    return getDomInfo(__blk).dom;
}
inline std::vector <block *> &getFrontier(block *__blk) {
    return getDomInfo(__blk).fro;
}


} // namespace dark::IR
