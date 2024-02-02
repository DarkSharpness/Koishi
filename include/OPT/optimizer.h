#pragma once
#include "ASTbase.h"
#include "IRbase.h"

namespace dark {

struct optimizer : hidden_impl {
    static void init(int, char **);
    static void optimize_AST(AST::ASTbuilder *);
    static void optimize_IR(IR::IRbuilder *);
};

} // namespace dark

