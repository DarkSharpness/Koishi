#include "bitset.h"
#include "IRnode.h"
#include "dominant.h"
#include "DCE/unreachable.h"

#include <ranges>
#include <algorithm>

namespace dark::IR {

using bitset = dark::dynamic_bitset;

struct domNode {
    bitset dom;     // Dominator set
    bitset fro;     // Frontier set
    std::size_t id; // Unique id
};

/* Local method. */
static domNode &getDomNode(block *__p) { return *__p->get_ptr <domNode>(); }

/* Work out all the __x: __x has frontier __node */
static void initFrontier(block *__node, std::size_t __id, std::size_t __n) {
    bool __flag {};
    bitset __fro { __n };
    const auto &__info = getDomNode(__node);
    for (auto __prev : __node->prev) {
        auto __tmp = getDomNode(__prev).dom;
        __flag |= __tmp[__id];
        __tmp &= __info.dom;
        __tmp ^= getDomNode(__prev).dom;
        __fro |= __tmp;
    }
    __fro[__id] = __flag;
    getDomNode(__node).fro = std::move(__fro);
}


/**
 * @brief Initialize the edge of the function.
 * Specially, a dummy node is added to link all the return statements.
 */
void dominantMaker::initEdge(function *__func) {
    constexpr auto __link = [](block *__prev, block *__next) {
        __prev->next.push_back(__next);
        __next->prev.push_back(__prev);
    };

    for (auto &__p : __func->data)
        __p->next.clear(), __p->prev.clear();

    dummy.next.clear(), dummy.prev.clear();
    dummy.set_ptr(new domNode);

    for (auto &__p : __func->data) {
        if (auto *__br = __p->flow->as <branch_stmt>()) {
            __link(__p, __br->branch[0]);
            __link(__p, __br->branch[1]);
        }
        else if (auto *__jump = __p->flow->as <jump_stmt>())
            __link(__p, __jump->dest);
        else if (__p->flow->as <return_stmt>())
            __link(__p, &dummy);
        __p->set_ptr(new domNode);
    }
}

void dominantMaker::makePostOrder(block *__entry) {
    if (!visited.insert(__entry).second) return;
    for (auto *__p : __entry->next) makePostOrder(__p);
    rpo.push_back(__entry); // Reverse post order
}

dominantMaker::dominantMaker(function *__func, bool __is_post) {
    if (__func->is_unreachable()) return;
    initEdge(__func);

    block *__entry = __func->data.front();

    if (__is_post) {
        /* Post dominant: Deal with that in a similar manner. */
        __entry = &dummy;
        dummy.next = std::move(dummy.prev);
        for (auto &__p : __func->data) std::swap(__p->next, __p->prev);
    }

    makePostOrder(__entry);
    std::reverse(rpo.begin(), rpo.end());
    iterate(__entry);

    buildFrontier();
    if (__is_post) {
        /* Post dominant: Swap back prev and next. */
        for (auto &__p : __func->data) std::swap(__p->next, __p->prev);
    }
    removeDummy();
}

/**
 * Building the dominator tree iteratively. 
 */
void dominantMaker::iterate(block *__entry) {
    std::size_t __id = 0;
    for (auto __p : rpo) getDomNode(__p).id = __id++;

    bitset __all { rpo.size() };
    __all.set(); // Set all bits to 1

    for (auto __p : rpo | std::views::drop(1))
        getDomNode(__p).dom = __all;
    /* Entry is special. */
    getDomNode(__entry).dom.resize(rpo.size());
    getDomNode(__entry).dom.set(0);

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
    // Intersect all dominator sets of predecessors.
    auto __dom = getDomNode(*__beg).dom;
    for (auto __prev : std::span { __beg + 1, __end })
        __dom &= getDomNode(__prev).dom;
    
    auto &__info = getDomNode(__node);
    __dom.set(__info.id);

    if (__dom == __info.dom) return false;
    return __info.dom = std::move(__dom), true;
}

void dominantMaker::removeDummy() {
    constexpr auto __erase = [](std::vector <block *> &__vec) {
        auto __iter = std::find(__vec.rbegin(), __vec.rend(), &dummy);
        if (__iter != __vec.rend()) __vec.erase(__iter.base() - 1);
    };
    for (auto *__p : rpo) {
        __erase(__p->next);
        __erase(__p->prev);
        __erase(getDomSet(__p));
        __erase(getFrontier(__p));
    }
    __erase(rpo);
}

void dominantMaker::clean(function *__func) {
    for (auto &__p : __func->data) {
        delete __p->get_ptr <_Info_t>();
        __p->set_ptr(nullptr);
    }
    delete dummy.get_ptr <_Info_t>();
    dummy.set_ptr(nullptr);
}

void dominantMaker::buildFrontier() {
    std::size_t __id = 0;
    for (auto __node : rpo) initFrontier(__node, __id++, rpo.size());

    __id = 0;
    /* Translate to normal node. */
    std::vector <_Info_t> __new (rpo.size());
    for (auto __node : rpo) {
        auto &__info = getDomNode(__node);
        std::size_t __n = __info.fro.find_first();
        while (__n != bitset::npos) {
            __new[__n].fro.push_back(__node);
            __n = __info.fro.find_from(__n + 1);
        }
        std::size_t __m = __info.dom.find_first();
        while (__m != bitset::npos) {
            __new[__id].dom.push_back(rpo[__m]);
            __m = __info.dom.find_from(__m + 1);
        }
        ++__id;
        delete &__info;
    }

    __id = 0;
    for (auto __node : rpo)
        __node->set_ptr(new _Info_t {std::move(__new[__id++])});
}


} // namespace dark::IR
