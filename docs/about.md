# About DarkSharpness Mx Compiler Project

## Frontend

We use anltr4 to generate the lexer and parser for the mx language. The grammar is defined in `src/antlr4/MxLexer.g4` and `src/antlr4/MxParser.g4`.

## IR

We use LLVM IR as the IR of our compiler. We will make sure that the IR in SSA form, so that we can perform some optimizations on it efficiently.

## Backend

We plan to use graph coloring register allocation algorithm to allocate registers for variables. In our last project, we use a DarkSharpness-modified linear scan algorithm, which is both slow and not optimal. Therefore, we decide to use graph coloring algorithm this time.

However, we have not decided yet which to use. Maybe register allocation on SSA form is a good choice, but we are not sure about that.

## Optimizations

There are 3 parts of optimizations we plan to do:

1. Optimizations on AST.
2. Optimizations on IR.
3. Optimizations on ASM.

The second part is what we will focus on. We plan to do the following optimizations:

1. Constant folding (on AST).
2. Early loop formation (on AST).
3. Sparse conditional constant propagation (on IR).
4. Aggressive dead code elimination (on IR).
5. Common subexpression elimination (on IR).
6. Peephole optimization (on IR, maybe also on ASM).
7. Strength reduction (on IR, maybe also on ASM).
8. Loop invariant code motion (on IR).
9. Loop induction variable optimization (on IR).
10. Function inlining (on IR).
11. Control flow graph simplification (on IR).
12. Scalar replacement of aggregates (on IR).
13. Tail call optimization (on ASM, maybe also on IR).
14. Register allocation (on ASM).
15. Specialized code generation based on Mx (on every part).

In our last project, we have implemented 1,3,4,5,6,7,10,11,13,14. We also partly implemented 12 (which effectively converts some malloc to alloca), and tried to identify a general pattern of loop (but failed). We will try to implement the rest of them in this project.

What's more, the instruction selection in our last project is not optimal. We will try to improve it in this project.
