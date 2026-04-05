# General Overview

## Goals

Quecto is meant to be a experimental systems language but as it stands the compiling process is whats being developed before any
high-level constructs are made or added. There are no explicit design constraints and choices are made semi-arbitrarily.

Currently the desired grammar looks like this:
```
  proc divide (dividend: i32, divisor: i32) => (quotient: i32, remainder: i32) {
    return (dividend / divisor, dividend % divisor);
  }

  proc main () => (status: i32) {
    i := 0;
    while (i < 30) {
      q, r := divide(i, 15);
      if (q % 3 == 0)
        print("bow");
      
      if (r == 2 or r == 9)
        print("wow");
      else
        print("now");
    }
    
    return 0;  
  }
```
However, the only language features implemented are:
  - procedures (without multiple return values or named return values)
  - thus no multiple assign
  - there is no system libraries yet (but we plan on using libc functions for now with an extern keyword)
  - there is a analysis and preliminary type system in place though the size and conventions (u32 vs i32) are not yet used in IR/code gen
  - there is no and/or functionality yet
  - there are no bitwise operators yet

The list can go on and on.

## Compiler

Currently the Quecto Compiler has 5 stages:
  - Texer
  - Parsing
  - Analysis
  - IR Generation
  - Code Generation

### Lexing

Nothing out of the ordinary, it is a handwritten tokenizer/lexer. It uses GPERF to generate a hashmap to detect keywords and
other reserved words. One goal is to write our own GPERF, though it is not a priority.

Related Files:
  - `include/tokenizer.h`
  - `src/tokenizer.c`
  - `src/keywords.gperf`
  
### Parsing

Generation of an AST is done via Recursive Descent Parsing.

Related Files:
  - `include/parser.h`
  - `src/parser.c`
  - `include/ast.h`
  - `include/ast.c`

### Analysis

Parsing merely structurally creates the AST but assigns no meaning to symbols but this can be derived through analyzing the structure
to see if it is valid.

Related Files:
  - `include/analysis.h`
  - `src/analysis.c`

### IR Generation

Quecto uses its own Intermediate Representation that currently has roughly 22 different OPCODEs but these are subject to change. This
stage is dependent on Analysis such that the IR code generated is valid and it is also depedent on the Symbol Table derived in Analysis.

Related Files:
  - `include/bytecode.h`
  - `src/bytecode.c`

### Code Generation

Currently, Quecto is hardcoded to output to NASM assembly to be compiled to x86_64. There are future plans for a slew of backends. Most
of these backends essentially translate the IR to the desired assembly convention though there are some adherence rules based on that
assembly's conventions.

Related Files:
  - `include/codegen.h`
  - `src/codegen.c`
