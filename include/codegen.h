#ifndef CODEGEN_H
#define CODEGEN_H

#include "ir.h"

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
    struct {
      int base; // defaults to rbp if bytecode operand doesnt specify
      int stack;
      int index;
      int size;
    };
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

// typedef struct {
//   const char *procedure_name;
//   Operand args[MAX_PARAMS];
//   size_t arg_count;
//   size_t current_instruction;
//   size_t stack_offset;
// } CodegenContext;

typedef struct {
  MachineInstr *items;
  int count;
  int capacity;
} MachineCode;

typedef struct {
  Arena *scratch;
  MachineCode output;
  VregInfoTable *vregs;
  SlotTable *slots;
  int *labels;

  int *args;
  int arg_count;

  size_t stackframe;
  size_t *stack_from_slot;
  // CodegenContext ctx;
} CodegenInterface;

typedef void (*EmitFn)(CodegenInterface *iface, Instr instr);
typedef void (*CalculateFn)(CodegenInterface *iface);
typedef void (*EmitProcFn)(CodegenInterface *face, Procedure *procedure);
typedef Bytecode (*AdhereFn)(Bytecode bytecode);
typedef void (*FPrintFn)(FILE *out, MachineCode *code, size_t bsize, int block_pos[bsize]);
typedef void (*EmitProgramDataFn)(FILE *out, Program *program);

typedef struct {
  AdhereFn adhere_bytecode_to_spec;
  CalculateFn calculate_offsets;
  EmitProgramDataFn emit_symbols;
  EmitProgramDataFn emit_entry;
  AdhereFn adhere_bytecode;
  EmitProcFn emit_prologue;
  EmitProcFn emit_epilogue;
  EmitFn emit_machine_code_from[OPCODE_COUNT];
  FPrintFn print_machine_code;
} CodegenBackend;

// functions that every backend will need to implement
// MachCode instruction_selection(Bytecode bytecode, PhysRegs *pregs);
// This function essentially performs any and all transformations to
// the bytecode that are necessary before register allocation occurs
// (e.g. 3AC to 2AC conversion for x64, respecting calling conventions, etc;)

void compile_program_with(FILE *out, CodegenBackend *backend, Program *program);
void print_machine_code(MachineCode *code, int indent);

#endif
