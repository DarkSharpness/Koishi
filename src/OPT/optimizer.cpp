#include "optimizer.h"
#include "folder.h"


namespace dark {


void optimizer::init(int argc, char **argv) {
    // TODO: Complete parsing arguments.
}

void optimizer::optimize_AST(AST::ASTbuilder *ctx) {
    AST::constantFolder { ctx };
}

void optimizer::optimize_IR(IR::IRbuilder *ctx) {

}


} // namespace dark
