#ifndef CODEGEN_H
#define CODEGEN_H

#include "bytecode.h"

typedef enum {
  MOPERAND_INVALID = 0,
  MOPERAND_REG,
  MOPERAND_STACK,
  MOPERAND_IMM,
  MOPERAND_LABEL, 
} MachineOperandType;

typedef struct {
  MachineOperandType type;
  union {
    int reg;
    int stack;
    int imm;
    const char *label;
  };
} MachineOperand;

typedef struct {
  int instruction;
  MachineOperand dest;
  MachineOperand arg1;
  MachineOperand arg2;
} MachineInstr;

typedef struct {
  const char *procedure_name;
  Operand args[MAX_PARAMS];
  size_t arg_count;
  size_t current_instruction;
  size_t stack_offset;
} CodegenContext;

typedef struct {
  MachineInstr *items;
  int count;
  int capacity;
} MachineCode;

typedef struct {
  MachineCode output;
  LocationArray location;
  IntervalArray interval;
  CodegenContext ctx;
} CodegenInterface;

typedef void (*EmitFn)(CodegenInterface *iface, Instr instr);
typedef Bytecode (*AdhereFn)(Bytecode bytecode, PhysRegs *pregs);
typedef void (*PrintFn)(MachineCode *code);

typedef struct {
  AdhereFn adhere_bytecode_to_spec;
  EmitFn emit_machine_code_from[OPCODE_COUNT];
  PrintFn print_machine_code;
} CodegenBackend;

// functions that every backend will need to implement
// MachCode instruction_selection(Bytecode bytecode, PhysRegs *pregs);
// This function essentially performs any and all transformations to
// the bytecode that are necessary before register allocation occurs
// (e.g. 3AC to 2AC conversion for x64, respecting calling conventions, etc;)
void adhere_program(Program *program, PhysRegs *pregs);
Bytecode adhere_bytecode_to_machine_spec(Bytecode bytecode, PhysRegs *pregs);


void compile_program(FILE *out, Program *program);
void emit_procedure_assembly(FILE *out, Procedure procedure);

void compile_program_with(CodegenBackend *backend, Program *program);
void print_machine_code(MachineCode *code, int indent);

// TODO: locations should probably not be passed in here and instead we have some
// codegen state struct or globals containing that type of info

#endif
