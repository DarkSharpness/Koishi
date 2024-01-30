#include "IRbuilder.h"
#include "ASTbuilder.h"
#include "IRnode.h"
#include <sstream>


namespace dark::IR {

std::string IRbuilder::IRtree() const {
    std::stringstream ss;
    for (auto &&[__str, __class] : class_map)
        if (auto *__ptr = dynamic_cast <custom_type *> (__class))
            ss << __ptr->data();

    ss << '\n';

    for (auto &&__var : global_variables) {
        ss << __var.data();
        if (auto *__lit = __var.const_init)
            ss << " = " << __lit->IRtype() << ' ' << __lit->data();
    }

    for (auto &&__func : global_functions)
        __func.print(ss);

    for (auto &&[__name, __var] : IRpool::str_pool) {
        ss << __var.data();
        if (auto *__lit = __var.const_init)
            ss << " = " << __lit->IRtype() << ' ' << __lit->data();
    }

    return ss.str();
}


} // namespace dark::IR
