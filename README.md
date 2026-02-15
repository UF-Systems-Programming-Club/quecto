# Quecto Programming Language

This is currently only a simple calculator than can execute expressions with binary operators
\+, \-, \*, and /. However, it is structured in a way to be incrementally added to. There is a tokenizer,
parser, and tree-walking interpreter already in place that merely need to be added on to in order to extend
the language ever further.

It supports operations between floating point and unsigned integers and currently promotes unsigned integers to floats if mixed in operations.

For assembly output, I only added addition for now. The assembly program exits with the result of the expression. If your shell or editor
does not automatically display the exit code upon process exit, run
```bash
echo $?
```
to view the exit code.

## Intermediate Language Description
