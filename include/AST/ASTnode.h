#pragma once
#include "ASTbase.h"

/* Node part */
namespace dark::AST {

/* Array access */
struct subscript_expr final : expression {
    expression *expr;
    expression_list subscript;
    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitSubscript(this); }
};

/* Function calls. */
struct function_expr final : expression {
    expression *    func;
    expression_list args;
    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitFunction(this); }
};

/* A unary expression. */
struct unary_expr final : expression {
    operand_t   op;     // Suffix "++" will be denoted as "+++"
    expression *expr;
    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitUnary(this); }
};

/* A binary expression */
struct binary_expr final : expression {
    operand_t   op;
    expression *lval;
    expression *rval;
    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitBinary(this); }
};

/* A ternary expression. */
struct ternary_expr final : expression {
    expression *cond;
    expression *lval;   // Left expression, if cond is true.
    expression *rval;   // Right expression, if cond is false.
    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitTernary(this); }
};

/* Member access. */
struct member_expr final : expression {
    expression *expr;   // Expression to access.
    std::string name;   // Name of the member.
    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitMember(this); }
};

/* Operator new expression. (May contains array) */
struct construct_expr final : expression {
    expression_list subscript;
    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitConstruct(this); }
};

/* Atomic expression (identifier). */
struct atomic_expr final : expression {
    std::string name;
    identifier *real {};    // Pointer to the identifier.
    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitAtomic(this); }
};

/* Literal constants. */
struct literal_expr final : expression {
    enum {
        NUMBER,
        STRING,
        _NULL_,
        _BOOL_,
    } sort; // Type of the literal.
    std::string name;
    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitLiteral(this); }
};

} // namespace dark::AST


/* Statement part */
namespace dark::AST {

/* For loop. */
struct for_stmt final : statement, loop_type {
    statement  *init {};
    expression *cond {};
    expression *step {};
    statement  *body {};
    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitFor(this); }
};

/* While loop. */
struct while_stmt final : statement, loop_type {
    expression *cond {};
    statement  *body {};
    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitWhile(this); }
};

/* Flow controller. */
struct flow_stmt final : statement {
    enum {
        RETURN,
        BREAK,
        CONTINUE,
    } sort; // Type of the flow controller.
    expression *expr {};    // Return value (if any)
    union {
        function   *func {} ;   // Function it belongs to.
        loop_type  *loop    ;   // Loop it belongs to.
    };

    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitFlow(this); }
};

/* Code block. */
struct block_stmt final : statement {
    std::vector <statement *> stmt;

    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitBlock(this); }
};

/* Branch statement, which may contain if and elses. */
struct branch_stmt final : statement {
    struct branch_t {
        expression *cond {};    // Should never be null.
        statement  *body {};    // Should never be null, even if empty.
    };

    std::vector <branch_t> branches;
    statement *else_body {};    // May be null, when there is no else.

    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitBranch(this); }
};

/* Comma separated expression. */
struct simple_stmt final : statement {
    expression_list expr;
    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitSimple(this); }
};

} // namespace dark::AST


/* Definition part */
namespace dark::AST {


struct variable_def final : definition, statement {
    typeinfo        type;   // Type of the variables.
    variable_list   vars;   // Variable name-initializer pairs.
    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitVariableDef(this); }
};

struct function_def final : definition, identifier {
    argument_list args;     // Arguments of the function.
    statement    *body {};  // Body of the function. If null, it's built-in.
    std::vector <variable *> locals;    // Local variables.
    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitFunctionDef(this); }
    bool is_builtin() const noexcept { return body == nullptr; }
    bool is_constructor() const noexcept { return name.empty(); }
};

struct class_def final : definition {
    std::string     name;   // Name of the class.
    definition_list member; // Member variables and functions.
    void print() const override;
    void accept(ASTbase *visitor) override { visitor->visitClassDef(this); }
};


} // namespace dark::AST
