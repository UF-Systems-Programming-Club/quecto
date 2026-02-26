#ifndef CODEGEN_H
#define CODEGEN_H

InstList adhere_ir_to_machine_spec(InstList ir);
void generate_assembly_from_ir(FILE *out, InstList ir, LocationArray location);

#endif
