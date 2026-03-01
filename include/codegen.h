#ifndef CODEGEN_H
#define CODEGEN_H

BytecodeArray adhere_ir_to_machine_spec(BytecodeArray ir);
void generate_assembly_from_ir(FILE *out, BytecodeArray ir, LocationArray location);

#endif
