# About DarkSharpness Mx Compiler Project

A compiler, written in C++, which can Mx language source code to RISCV assembly.

## How to run it

Our compiler is written in C++, and we use CMake to manage the project. To build the project, you need to have CMake installed on your computer. The compiler should support `C++20`, with `format`, `range` and `concepts` features. We strongly recommend you to use `gcc-13` or later, since it's the environment we use to develop the project.

Also, we need antlr4 to generate the lexer and parser for the mx language.

Use the following commands to check the version of the tools:

```bash
g++ -v          # At least 13.1.0
cmake --version # At least 3.20.0
antlr4          # At least 4.9.2, recommend 4.13.0
```

Meeting these requirements, you can build the project by running the following commands:

```bash
./configure
```

The binary will be generated in the bin directory. Don't forget to give permission to the `configure` script.

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
3. Optimizations on MIR.

The second part is what we will focus on. We plan to do the following optimizations:

1. [Done]   Constant Folding (on AST).
2. [Done]   Early Loop Formation (on AST).
3. [Done]   Sparse Conditional Constant Propagation (on IR).
4. [Done]   Aggressive Dead Code Elimination (on IR).
5. [Done]   Common Subexpression Elimination (on IR; implemented by GVM).
6. [Done]   Algebraic simplification (on IR).
7. [Busy]   Strength reduction (on IR, maybe also on MIR).
8. [Done]   Loop invariant code motion (on IR; implemented by GCM).
9. [Todo]   Loop induction variable optimization (on IR).
10. [Todo]  Function inlining (on IR).
11. [Done]  Control flow graph simplification (on IR; covered by ADCE).
12. [Todo]  Scalar replacement of aggregates (on IR).
13. [Todo]  Tail call optimization (on ASM, maybe also on IR).
14. [Todo]  Register allocation (on ASM).
15. [Todo]  Specialized code generation based on Mx (on every part).

In our last project, we have implemented 1,3,4,5,6,7,10,11,13,14. We also partly implemented 12 (which effectively converts some malloc to alloca), and tried to identify a general pattern of loop (but failed). We will try to implement the rest of them in this project.

What's more, the instruction selection in our last project is not optimal. We will try to improve it in this project.
