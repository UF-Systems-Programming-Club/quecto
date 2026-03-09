#ifndef CODEGEN_H
#define CODEGEN_H

#include "bytecode.h"

// functions that every backend will need to implement

// This function essentially performs any and all transformations to
// the bytecode that are necessary before register allocation occurs
// (e.g. 3AC to 2AC conversion for x64, respecting calling conventions, etc;)
void adhere_program(Program *program, PhysRegs *pregs);
Bytecode adhere_bytecode_to_machine_spec(Bytecode bytecode, PhysRegs *pregs);


void compile_program(FILE *out, Program *program);
void emit_procedure_assembly(FILE *out, Procedure procedure);

// TODO: locations should probably not be passed in here and instead we have some
// codegen state struct or globals containing that type of info

// Each of these turn the VM bytecode instruction into the assembly instruction(s)
// for the backend ISA and appends them to the end of out
void emit_add(FILE *out, Instr instr, LocationArray location);
void emit_sub(FILE *out, Instr instr, LocationArray location);
void emit_mul(FILE *out, Instr instr, LocationArray location);
void emit_div(FILE *out, Instr instr, LocationArray location);
void emit_load(FILE *out, Instr instr, LocationArray location);
void emit_store(FILE *out, Instr instr, LocationArray location);
void emit_copy(FILE *out, Instr instr, LocationArray location);
void emit_loadi(FILE *out, Instr instr, LocationArray location);
void emit_ret(FILE *out, Instr instr, LocationArray location);


#endif
