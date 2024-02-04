#pragma once
#include <queue>
#include <unordered_set>

namespace dark {

template <typename _Tp>
struct work_queue : public std::queue <_Tp> , protected std::unordered_set <_Tp> {
  private:
    using _Set_t        = std::unordered_set <_Tp>;
    using _List_t       = std::queue <_Tp>;
    using _Container_t  = typename _List_t::container_type;

  public:
    _Set_t & set() { return *this; }
    /* Return the underlying container. */
    _Container_t & container() { return _List_t::c; }
    /* Add an element to the queue if it is not already set. */
    void push(const _Tp & __t) { if (this->insert(__t).second) _List_t::push(__t); }
    /* Add an element to the queue if it is not already set. */
    void push(_Tp && __t) { if (this->insert(__t).second) _List_t::push(std::move(__t)); }
    /* Force to push an element to the queue. */
    void force_push(_Tp __t) { this->insert(__t); _List_t::push(std::move(__t)); } 
    /* Pop the first element and return it. */
    [[nodiscard]]_Tp pop_value() {
        _Tp __t = std::move(_List_t::front()); _List_t::pop();
        return __t;
    }
    std::size_t size() const { return _List_t::size(); }
    bool empty() const { return _List_t::empty(); }
    bool contains(const _Tp & __t) const { return _Set_t::contains(__t); }
    void clear() { _List_t::clear(); _Set_t::clear(); }
};


} // namespace dark
