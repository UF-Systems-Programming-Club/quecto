# Quecto Programming Language

This is currently a simple programming language that implements procedures with multiple parameters and a single
return value. It also supports if, while and return statements. Currently a single file is tokenized, parsed into
an AST and transcribed into a custom IR. This IR currently only supports producing NASM assembly.

Currently, the result of the program can only be seen with the exit code as no syscalls or libc functions are yet
supported.

If your shell or editor does not automatically display the exit code upon process exit, run
```bash
echo $?
```
to view the exit code.

## Intermediate Language Description

Arithmetic instructions:
can utilize two registers, or a register and an imm (order matters, so has to be reg plus imm)
* add
* sub
* mul
* div

loadi
