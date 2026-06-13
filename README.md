# Quecto Programming Language

Quecto is a simple programming language that so far implements basic control-flow procedures like calls, branches and loops.

Right now, it uses a custom IR based on CFG+SSA which can be emitted into NASM x86-64 assembly.

It is heavily in development and only supports basic programs like those in `./examples`. It currently has limited testing as well, so correctness is not fully verified.
It supports external libc calls right now which can be seen in `./examples/array.q`. However, it requires the array "AAAA\0" be decayed into a pointer.

Currently, the result of the program can be seen with the exit code.


## Overview

See [Overview](./OVERVIEW.md).

## Contributing

See [Contributing](./CONTRIBUTING.md).

## Usage

```bash
  make
  ./main build examples/proc.q
  make out
  ./out
```

If your shell or editor does not automatically display the exit code upon process exit, run
```bash
echo $?
```

## WIP

Most recently, the IR was significantly refactored (i.e. linear bytecode -> CFG). Future work is to now be done on the front end of the language regarding AST parsing, analysis and AST->IR emission
so that the new backend can be taken advantage of. Of course, the backend is still very much a WIP but it is stronger than before and now implements dominance, phi insertion, etc. for SSA transformation.
Testing the intermediate representations between stages also needs to be implemented and similar with verifying example program outputs.
