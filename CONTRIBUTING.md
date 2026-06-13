# Contributing

If you want to contribute to Quecto and don't know where to start, here is a compilation of ideas of what to look at or do.

## Improvements

- Add more tests for the Parsing, Analysis and Emission Passes
- Verify the output of sample Quecto programs to make it easier to catch regressions
- Add debug flags to the binary such that intermediate AST and IR can be output into text files
- Improve the Parsing Pass by recovering from structurally incorrect code to parse the rest of the program so that multiple errors can be reported
- Improve the Analysis Pass by disallowing incorrect Quecto programs (i.e. referencing an rvalue expression)
- Improve the Analysis Pass by giving clearer error messages
- Cleanup the x86-64 backend for Linux
- Fill in x86-64 backend stubs for MacOS and Windows
- Improve all Passes by trying to prevent the compiler from crashing so that error that caused crash can be reported

## Next Steps

- Add an ARM backend to the Codegen Pass
- Add more syntax constructs
  - Multiple assignments (i.e. a, b = 2, c)
  - Syntactic sugar (i.e. a += 2 instead of a = a + 2)
  - More operators (i.e and/or are unimplemented, != is unimplemented, bitwise operators are unimplemented)
- Allow for multiple return values
  - Requires new Calling Convention semantics such that the pseudo SystemV functions (i.e LibC) don't clobber with the Quecto Calling Convention
  - Analysis would need to be able to prevent assignment to a single variable from a multi-value return
- Add division via the new IR and regalloc so that the clobbered registers (for x86-64) don't affect correctness of the program
- Add a new pass between parsing and IR emission to lower the AST into something closer to IR but contains analysis semantics such as casting, pointer decay, etc.
- Refactor the Symbol Table
- Add String Literals
- Add Global/Static Variables
- Create 'ir_str' similar to the current 'ast_str' to dynamically build strings using arena semantics
