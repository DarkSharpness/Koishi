#include "IRnode.h"
#include "dominant.h"
#include "DCE/unreachable.h"

#include <ranges>
#include <algorithm>

namespace dark::IR {


static void __printDebug(block *__block) {
    std::cerr << __block->name << ":";
    std::cerr << "\n\tFrontier:\t";
    for (auto __p : getFrontier(__block))
        std::cerr << __p->name << " ";
    std::cerr << "\n\tDomed by:\t";
    for (auto __p : getDomSet(__block))
        std::cerr << __p->name << " ";
    std::cerr << '\n';
}

/**
 * @return Whether x dominates y.
 */
static bool __isDom(block *__x, block *__y) {
    auto &__dom = getDomSet(__y);
    return std::binary_search(__dom.begin(), __dom.end(), __x);
}

static void __link(block *__prev, block *__next) {
    __prev->next.push_back(__next);
    __next->prev.push_back(__prev);
}

void dominantMaker::initEdge(function *__func) {
    for (auto &__p : __func->data)
        __p->next.clear(), __p->prev.clear();

    dummy.next.clear(), dummy.prev.clear();
    dummy.set_impl_ptr(new _Info_t);

    for (auto &__p : __func->data) {
        if (auto *__br = dynamic_cast <branch_stmt *> (__p->flow)) {
            __link(__p, __br->branch[0]);
            __link(__p, __br->branch[1]);
        }
        else if (auto *__jump = dynamic_cast <jump_stmt *> (__p->flow))
            __link(__p, __jump->dest);
        else if (dynamic_cast <return_stmt *> (__p->flow))
            __link(__p, &dummy);

        __p->set_impl_ptr(new _Info_t);
    }
}

void dominantMaker::makeRpo(block *__entry) {
    if (!visited.insert(__entry).second) return;
    for (auto *__p : __entry->next) makeRpo(__p);
    rpo.push_back(__entry); // Reverse post order
}

dominantMaker::dominantMaker(function *__func, bool __is_post) {
    initEdge(__func); dummy.name = ".dummy";

    unreachableRemover {__func};
    if (__func->is_unreachable()) return;

    block *__entry = __func->data.front();

    /* Post dominant: Deal with that in a similar manner. */
    if (__is_post) {
        __entry = &dummy;
        for (auto &__p : __func->data) std::swap(__p->next, __p->prev);
        std::swap(dummy.next, dummy.prev);
    }

    makeRpo(__entry);
    std::reverse(rpo.begin(), rpo.end());
    iterate(__entry);

    removeDummy(__func);
    for (auto __node : rpo) // Enumerate "y"
        for (auto __prev : __node->prev)
            for (auto __temp : getDomSet(__prev))  // Possible "x"
                /**
                 * Frontier y of x:
                 *  1. x dominates one predecessor of y.
                 *      <=> x is in domSet of one predecessor of y.
                 *  2. x don't dominate y , x = y
                 *      <=> x = y , or x is not in domSet of y.
                */
                if (__node == __temp || !__isDom(__temp, __node))
                    getFrontier(__temp).push_back(__node);

    for (auto __node : rpo) {
        auto &__set = getFrontier(__node);
        std::sort(__set.begin(), __set.end());
        __set.resize(std::unique(__set.begin(), __set.end()) - __set.begin());
    }
}

/**
 * Building the dominator tree iteratively. 
 */
void dominantMaker::iterate(block *__entry) {
    std::vector <block *> __tmp = rpo;
    std::sort(__tmp.begin(), __tmp.end());
    for (auto __p : rpo | std::views::drop(1))
        getDomSet(__p) = __tmp;
    getDomSet(__entry) = {__entry};

    bool __changed;
    do {
        __changed = false;
        for (auto *__p : rpo | std::views::drop(1))
            __changed |= update(__p);
    } while (__changed);
}

bool dominantMaker::update(block *__node) {
    runtime_assert(__node->prev.size() > 0, "wtf?");
    auto __beg = __node->prev.begin();
    auto __end = __node->prev.end();
    std::vector <block *> __dom = getDomSet(*__beg);
    std::vector <block *> __tmp;

    /* Intersect the dominator of all predecessors. */
    for (auto *__p : std::span {__beg + 1, __end}) {
        auto &__cur = getDomSet(__p);
        std::set_intersection(
            __dom.begin(), __dom.end(),
            __cur.begin(), __cur.end(),
            std::back_inserter(__tmp)
        );
        std::swap(__tmp, __dom); __tmp.clear();
    }

    auto __iter = std::lower_bound(__dom.begin(), __dom.end(), __node);
    if (__iter == __dom.end() || *__iter != __node)
        __dom.insert(__iter, __node);

    if (__dom != getDomSet(__node)) {
        return getDomSet(__node) = std::move(__dom), true;
    } else {
        return false;
    }
}

void dominantMaker::removeDummy(function *__func) {
    constexpr auto __erase = [](std::vector <block *> &__vec) {
        auto __iter = std::find(__vec.rbegin(), __vec.rend(), &dummy);
        if (__iter != __vec.rend()) __vec.erase(__iter.base() - 1);
    };
    for (auto &__p : __func->data) {
        __erase(__p->next);
        __erase(__p->prev);
        __erase(getDomSet(__p));
    }
    __erase(rpo);
}

void dominantMaker::clean(function *__func) {
    for (auto &__p : __func->data) {
        delete __p->get_impl_ptr <_Info_t>();
        __p->set_impl_ptr(nullptr);
    }
    delete dummy.get_impl_ptr <_Info_t>();
    dummy.set_impl_ptr(nullptr);
}

} // namespace dark::IR
