#pragma once
#include "IRbase.h"

namespace dark::IR {

struct compare_stmt final : statement {
    temporary  *dest;
    definition *lval;
    definition *rval;
    enum compare_op {
        EQ, NE, LT, LE, GT, GE
    } op;

    explicit compare_stmt(temporary *, definition *, definition *, compare_op);

    void accept(IRbase *v)  override { v->visitCompare(this); }
    std::string data()      const override;
    temporary *get_def()    const override;
    _Def_List  get_use()    const override;
    void update(definition *, definition *) override;
};

struct binary_stmt final : statement {
    temporary  *dest;
    definition *lval;
    definition *rval;
    enum binary_op {
        ADD, SUB, MUL, DIV, MOD, SHL, SHR, AND, OR, XOR
    } op;

    explicit binary_stmt(temporary *, definition *, definition *, binary_op);

    void accept(IRbase *v)  override { v->visitBinary(this); }
    std::string data()      const override;
    temporary *get_def()    const override;
    _Def_List  get_use()    const override;
    void update(definition *, definition *) override;
};

struct jump_stmt final : flow_statement {
    block *dest;    // Next block

    explicit jump_stmt(block *);

    void accept(IRbase *v)  override { v->visitJump(this); }
    std::string data()      const override;
    _Def_List  get_use()    const override;
    void update(definition *, definition *) override;
};

struct branch_stmt final : flow_statement {
    definition *cond;   // Condition
    block *branch[2];   // br[0] false, br[1] true

    explicit branch_stmt(definition *, block *[2]);

    void accept(IRbase *v)  override { v->visitBranch(this); }
    std::string data()      const override;
    _Def_List  get_use()    const override;
    void update(definition *, definition *) override;
};

struct call_stmt final : statement {
    temporary *dest {};     // Return value
    function  *func {};     // Function to call.
    std::vector <definition *> args;    // Arguments

    explicit call_stmt(temporary *, function *);

    void accept(IRbase *v)  override { v->visitCall(this); }
    std::string data()      const override;
    temporary *get_def()    const override;
    _Def_List  get_use()    const override;
    void update(definition *, definition *) override;
};

struct load_stmt final : memory_statement {
    temporary   *dest;    // Loaded value
    definition  *addr;    // Source address

    explicit load_stmt(temporary *, non_literal *);

    void accept(IRbase *v)  override { v->visitLoad(this); }
    std::string data()      const override;
    temporary *get_def()    const override;
    _Def_List  get_use()    const override;
    void update(definition *, definition *) override;
};

struct store_stmt final : memory_statement {
    definition  *src_;    // Stored value
    definition  *addr;    // Destination address

    explicit store_stmt(definition *, non_literal *);

    void accept(IRbase *v)  override { v->visitStore(this); }
    std::string data()      const override;
    temporary *get_def()    const override;
    _Def_List  get_use()    const override;
    void update(definition *, definition *) override;
};

struct return_stmt final : flow_statement {
    definition *retval {};  // Return value.
    function   *func {};    // Function to end.

    explicit return_stmt(definition *, function *);

    void accept(IRbase *v)  override { v->visitReturn(this); }
    std::string data()      const override;
    _Def_List  get_use()    const override;
    void update(definition *, definition *) override;
};

struct alloca_stmt final : statement {
    local_variable *dest;   // Allocated variable

    explicit alloca_stmt(local_variable *);

    void accept(IRbase *)   override { runtime_assert(false, "No allca visitor!"); }
    std::string data()      const override;
    temporary *get_def()    const override;
    _Def_List  get_use()    const override;
    void update(definition *, definition *) override;
};

/* Get elementptr. */
struct get_stmt final : statement {
    static constexpr std::size_t NPOS = -1;
    temporary  *dest;   // Result
    definition *addr;   // Source address

    definition *index   {};     // Index of the array
    std::size_t member  {NPOS}; // Member index

    explicit get_stmt(temporary *, definition *, definition *, std::size_t = NPOS);

    void accept(IRbase *v)  override { v->visitGet(this); }
    std::string data()      const override;
    temporary *get_def()    const override;
    _Def_List  get_use()    const override;
    void update(definition *, definition *) override;
};

struct unreachable_stmt final : flow_statement {
    explicit unreachable_stmt() = default;
    void accept(IRbase *v)  override { v->visitUnreachable(this); }
    std::string data()      const override;
    _Def_List  get_use()    const override;
    void update(definition *, definition *) override;
};

/* Phi function. Which is not a "real" statement. */
struct phi_stmt final : private statement {
    /* Cast to base type. */
    statement *to_base() { return static_cast <statement *> (this); }
    friend class central_allocator <statement>;
    struct entry {
        block      *from;
        definition *init;
    };
    using _Phi_List = std::vector <entry>;

    temporary *dest;    // Result
    _Phi_List  list;    // Phi entries

    explicit phi_stmt(temporary *);
    explicit phi_stmt(temporary *, _Phi_List);

    void accept(IRbase *v)  override { v->visitPhi(this); }
    std::string data()      const override;
    temporary *get_def()    const override;
    _Def_List  get_use()    const override;
    void update(definition *, definition *) override;
};

/**
 * @brief A custom visitor for IRblock.
 * @tparam _Rule A callable object.
 */
template <typename _Rule>
inline void visitBlock(block *__block, _Rule &&__rule) {
    for (auto &__stmt : __block->phi)    __rule(__stmt->to_base());
    for (auto &__stmt : __block->data)   __rule(__stmt->to_base());
    __rule(__block->flow->to_base());
}




} // namespace dark::IR
