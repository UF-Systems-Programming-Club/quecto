#ifndef CODEGEN_H
#define CODEGEN_H

#include "bytecode.h"
#include "backends/linux_x64.h"

// functions that every backend will need to implement
MachCode instruction_selection(Bytecode bytecode, PhysRegs *pregs);
// This function essentially performs any and all transformations to
// the bytecode that are necessary before register allocation occurs
// (e.g. 3AC to 2AC conversion for x64, respecting calling conventions, etc;)
void adhere_program(Program *program, PhysRegs *pregs);
Bytecode adhere_bytecode_to_machine_spec(Bytecode bytecode, PhysRegs *pregs);


void compile_program(FILE *out, Program *program);
void emit_procedure_assembly(FILE *out, Procedure procedure);

// TODO: locations should probably not be passed in here and instead we have some
// codegen state struct or globals containing that type of info

#endif
