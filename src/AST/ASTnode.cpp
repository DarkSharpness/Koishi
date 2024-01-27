#include "ASTnode.h"


namespace dark::AST {

node_allocator::~node_allocator() {
    for (auto *node : data) delete node;
}

} // namespace dark::AST
