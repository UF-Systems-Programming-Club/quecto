#ifndef CODEGEN_H
#define CODEGEN_H

// TODO: will either have to have macros or conditional compilation
// here instead of this
#include "backends/linux_x64.h"

// functions that every backend will need to implement
MachCode instruction_selection(Bytecode bytecode, PhysRegs *pregs);

// This function essentially performs any and all transformations to
// the bytecode that are necessary before register allocation occurs
// (e.g. 3AC to 2AC conversion for x64, respecting calling conventions, etc;)
Bytecode adhere_bytecode_to_machine_spec(Bytecode bytecode, PhysRegs *pregs);

void emit_assembly_from_bytecode(FILE *out, Bytecode bytecode, LocationArray location, IntervalArray intervals);

// TODO: locations should probably not be passed in here and instead we have some
// codegen state struct or globals containing that type of info

#endif
