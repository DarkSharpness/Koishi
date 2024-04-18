# Koishi Compiler

A compiler translating [Mx* language](https://github.com/ACMClassCourses/Compiler-Design-Implementation?tab=readme-ov-file#%E4%BA%8Cmx-%E8%AF%AD%E8%A8%80%E5%AE%9A%E4%B9%89) source code to RISC-V assembly.

- For general information about this project (e.g. how to run this project), please refer to [About](docs/about.md).
- For implementation details or a nice (hope so) tutorial , refer to [Detail](docs/detail.md)
- For information about me (?), please visit my [website](https://darksharpness.github.io)
- For latest information, please continue reading.

## Latest update 2024-04-18

Here to introduce the new project name: `Koishi Compiler`. `Koishi` is a Japanese term meaning `small stone`, as well as the character `古明地(Komeiji) こいし(Koishi)` from the STG game Touhou Project.

The choice derived from a memorable experience in the previous [project](https://github.com/DarkSharpness/Mx-Compiler), where I witnessed the overwhelming force of collective wisdom. While acknowledging the impossibility of creating a Compiler that matches GCC or Clang in code quality, I still hope to explore the possibilities of optimization in this project, just as a small stone, creating my own ripple in the vast sea of compilers.

## 2024-04-17

Plan to write a new level of machine-level IR (MIR) for the compiler.

That's because certain level of optimization is hard to achieve with the current IR. For example, some constant load (`li`, `la` command) in loop can be moved out of the loop, but the current IR does not support this.

MIR requires instruction selection, and may exploit more optimization opportunities.
