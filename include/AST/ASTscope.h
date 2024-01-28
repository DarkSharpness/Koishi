#pragma once
#include "ASTbase.h"
#include <string>
#include <unordered_map>

namespace dark::AST {

struct scope {
  private:
    static identifier *recursive_search(const scope *__cur, std::string_view __name) {
        do {
            auto __iter = __cur->map.find(__name);
            if (__iter != __cur->map.end()) return __iter->second;  // Found
        } while ((__cur = __cur->prev));
        return nullptr; // Not found
    }
    using _Map_t = std::unordered_map <std::string_view, identifier *>;

  public:
    scope *prev {}; // Previous scope
    _Map_t  map {}; // Map of identifiers

    /**
     * @brief Tries to find the name in the scope.
     * @param __name Name to locate.
     * @return Pointer to identifier. If not found, return nullptr.
     */
    identifier *find(std::string_view __name) const {
        return recursive_search(this, __name);
    }

    /**
     * @brief Tries to insert a identifier into the scope.
     * @param __ptr Pointer of the identifier.
     * @return Whether the insertion was successful.
     */
    bool insert(identifier *__ptr) {
        return map.try_emplace(__ptr->name, __ptr).second;
    }
};

} // namespace dark::AST
