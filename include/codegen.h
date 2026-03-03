#ifndef CODEGEN_H
#define CODEGEN_H

// functions that every backend will need to implement

// This function essentially performs any and all transformations to
// the bytecode that are necessary before register allocation occurs
// (e.g. 3AC to 2AC conversion for x64, respecting calling conventions, etc;)
Bytecode adhere_bytecode_to_machine_spec(Bytecode bytecode, PhysRegs *pregs);

void generate_assembly_from_bytecode(FILE *out, Bytecode bytecode, LocationArray location);

// TODO: locations should probably not be passed in here and instead we have some
// codegen state struct or globals containing that type of info

// Each of these turn the VM bytecode instruction into the assembly instruction(s)
// for the backend ISA and appends them to the end of out
void gen_add(FILE *out, Instr instr, LocationArray location);
void gen_sub(FILE *out, Instr instr, LocationArray location);
void gen_mul(FILE *out, Instr instr, LocationArray location);
void gen_div(FILE *out, Instr instr, LocationArray location);
void gen_load(FILE *out, Instr instr, LocationArray location);
void gen_store(FILE *out, Instr instr, LocationArray location);
void gen_copy(FILE *out, Instr instr, LocationArray location);
void gen_loadi(FILE *out, Instr instr, LocationArray location);
void gen_ret(FILE *out, Instr instr, LocationArray location);


#endif
