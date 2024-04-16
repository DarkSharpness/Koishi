# DMC Project

DarkSharpness Mx Compiler.

A compiler translating [Mx language](https://github.com/ACMClassCourses/Compiler-Design-Implementation?tab=readme-ov-file#%E4%BA%8Cmx-%E8%AF%AD%E8%A8%80%E5%AE%9A%E4%B9%89) source code to RISCV assembly.

For general information about this project, please refer to [About](docs/about.md).

For implementation details, or a nice tutorial , refer to [Detail](docs/detail.md)

For information about me (?), please visit my [website](https://darksharpness.github.io)

For the latest information, continue reading.

## Latest update 2024-04-17

Plan to write a new level of machine IR for the compiler.

That's because certain level of optimization is hard to achieve with the current IR. For example, some constant load (`li` command) in loop can be moved out of the loop, but the current IR does not support this.

MIR requires instruction selection, and may exploit more optimization opportunities.
