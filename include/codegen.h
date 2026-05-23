#ifndef CODEGEN_H
#define CODEGEN_H

#include "ir.h"

#define LABEL_OPCODE 0

typedef enum {
  MOPERAND_INVALID = 0,
  MOPERAND_REG,
  MOPERAND_MEM,
  MOPERAND_IMM,
  MOPERAND_LABEL, 
} MachineOperandType;

typedef struct {
  int base;
  int offset;
  int index;
  int stride;
} MemAccess;

typedef struct {
  MachineOperandType type;
  union {
    int reg;
    MemAccess mem;
    int imm;
    int glbl;
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
  MachineInstr *items;
  int count;
  int capacity;
} MachineCode;

typedef struct {
  Arena *scratch;
  MachineCode output;
  VregInfoTable *vregs;
  SlotTable *slots;
  SymbolTable *globals;
  int *labels;

  int args[4];
  int arg_count;

  size_t stackframe;
  size_t *stack_from_slot;
} CodegenInterface;

typedef void (*EmitFn)(CodegenInterface *iface, Instr instr);
typedef char *(*MangleFn)(CodegenInterface *iface, const char *symbol);
typedef void (*CalculateFn)(CodegenInterface *iface);
typedef void (*EmitProcFn)(CodegenInterface *face, Procedure *procedure);
typedef Bytecode (*AdhereFn)(Bytecode bytecode);
typedef void (*FPrintFn)(FILE *out, MachineCode *code);
typedef void (*EmitProgramDataFn)(FILE *out, Program *program);

typedef struct {
  AdhereFn adhere_bytecode_to_spec;
  CalculateFn calculate_offsets;
  MangleFn mangle_symbols;
  EmitProgramDataFn emit_symbols;
  EmitProgramDataFn emit_entry;
  AdhereFn adhere_bytecode;
  EmitProcFn emit_prologue;
  EmitProcFn emit_epilogue;
  EmitFn emit_machine_code_from[OPCODE_COUNT];
  FPrintFn print_machine_code;
} CodegenBackend;

void compile_program_with(FILE *out, Arena *scratch, CodegenBackend *backend, Program *program);
void print_machine_code(MachineCode *code, int indent);

#endif
