#include "backends/linux_x64.h"
#include "bytecode.h"
#include "codegen.h"
  
#ifdef __MACH__
#define EXIT_STATUS "0x2000001"
#define ENTRY_SYMBOL "_main"
#else
#define EXIT_STATUS "60"
#define ENTRY_SYMBOL "_start"
#endif


#define CONCAT_HELPER(a, b) a##b
#define CONCAT(a, b) CONCAT_HELPER(a, b)

#define STR_HELPER(X) #X
#define STR(X) STR_HELPER(X)

#define REG_NAME(SIZE) CONCAT(registers_, CONCAT(SIZE, bit))
#define VIRT_TO_PHYS_REG(SIZE, ARG) registers[regsize_from_bytes(SIZE)][iface->location.items[(ARG)].register_index]
#define X64_INSTRUCTION(name) void emit_x64_from_ir_##name(CodegenInterface *iface, Instr instr)

#define MREG(x) ((MachineOperand){.type = MOPERAND_REG, .reg = (x) })
#define MSTK(o) ((MachineOperand){.type = MOPERAND_STACK, .stack = (o) })
#define MIMM(i) ((MachineOperand){.type = MOPERAND_IMM, .imm = (i) })
#define MLBL(l) ((MachineOperand){.type = MOPERAND_LABEL, .label = (l) })
#define MINV  ((MachineOperand){.type = MOPERAND_INVALID})

#define EMIT(out, op, d, a1, a2) \
  array_append( (out), ((MachineInstr) {.instruction = (op), .dest = (d), .arg1 = (a1), .arg2 = (a2)}) )

const char *x86_op_to_str[] = {
  [X64_ADD] = "add",
  [X64_SUB] = "sub",
  [X64_IMUL] = "imul",
  [X64_DIV] = "div",
  [X64_CMP] = "cmp",
  [X64_MOV] = "mov",
  [X64_RET] = "ret",
  [X64_JMP] = "jmp",
  [X64_JNE] = "jne",
  [X64_CALL] = "call",
  [X64_SETE] = "sete",
  [X64_SETL] = "setl",
  [X64_SETG] = "setg",
  [X64_SETLE] = "setle",
  [X64_SETGE] = "setge",
  [X64_JE] = "je",
  [X64_JL] = "jl",
  [X64_JG] = "jg",
  [X64_JLE] = "jle",
  [X64_JGE] = "jge",
  [X64_PUSH] = "push",
  [X64_POP] = "pop",
  [X64_LABEL] = "",
};

const char *x86_reg_to_str[] = {
  [x64_EAX] = "eax",
  [x64_ECX] = "ecx",
  [x64_EDX] = "edx",
  [x64_EDI] = "edi",
  [x64_ESI] = "esi",
  [x64_AL] = "al",
  [x64_CL] = "cl",
  [x64_DL] = "dl",
  [x64_DIL] = "dil",
  [x64_RAX] = "rax",
  [x64_RCX] = "rcx",
  [x64_RDX] = "rdx",
  [x64_RDI] = "rdi",
  [x64_RSP] = "rsp",
  [x64_RBP] = "rbp",
};

x64_Register registers[4][4] = {
  [0] = { x64_AL, x64_CL, x64_DL, x64_DIL },
  [1] = { },
  [2] = { x64_EAX, x64_ECX, x64_EDX, x64_EDI },
  [3] = { x64_RAX, x64_RCX, x64_RDX, x64_RDI }
};

x64_Register arg_registers_32bit[4] = {
  x64_EDI, x64_ESI, x64_EDX, x64_ECX
};


const X64_Opcode jmpCC_from_ir[OPCODE_COUNT] = {
  [OPCODE_JMP_NEQ] = X64_JNE,
  [OPCODE_JMP_EQ] = X64_JE,
  [OPCODE_JMP_LT] = X64_JL,
  [OPCODE_JMP_GT] = X64_JG,
  [OPCODE_JMP_LE] = X64_JLE,
  [OPCODE_JMP_GE] = X64_JGE,
};

const X64_Opcode setCC_from_ir[OPCODE_COUNT] = {
  [OPCODE_CMP_EQ] = X64_SETE,
  [OPCODE_CMP_LT] = X64_SETL,
  [OPCODE_CMP_GT] = X64_SETG,
  [OPCODE_CMP_LEQ] = X64_SETLE,
  [OPCODE_CMP_GEQ] = X64_SETGE,
};


inline int regsize_from_bytes(int bytes) {
  if (bytes <= 1) return 0;
  else if (bytes <= 2) return 1;
  else if (bytes <= 4) return 2;
  else if (bytes <= 8) return 3;
  UNREACHABLE("invalid bit size");
}


bool fprint_machine_operand(FILE *out, MachineOperand operand, bool leading) {
  switch (operand.type) {
    case MOPERAND_IMM:
      fprintf(out, "%d", operand.imm);
      break;
    case MOPERAND_REG:
      fprintf(out, "%s", x86_reg_to_str[operand.reg]);
      break;
    case MOPERAND_STACK:
      fprintf(out, "[rbp - %d]", operand.stack);
      break;
    case MOPERAND_LABEL:
      fprintf(out, "%s", operand.label);
      break;
    case MOPERAND_INVALID:
      return false;
  }
  if (leading) fprintf(out, ", ");
  return true;
}


void fprint_x64_machine_code(FILE *out, MachineCode *code) {
  for (int i = 0; i < code->count; i++) {
    MachineInstr instr = code->items[i];
    switch (instr.instruction) {
      case X64_LABEL:
        fprintf(out, "%s:\n", instr.dest.label);
        break;
      default:
        fprintf(out, "\t%s\t", x86_op_to_str[instr.instruction]);
        fprint_machine_operand(out, instr.dest, instr.arg1.type != MOPERAND_INVALID);
        fprint_machine_operand(out, instr.arg1, instr.arg2.type != MOPERAND_INVALID);
        fprint_machine_operand(out, instr.arg2, false);
        fprintf(out, "\n");
      break;
    }
  }
}

void emit_x64_prologue(CodegenInterface *iface, Procedure *procedure) {
  EMIT(iface->output, X64_PUSH, MINV, MREG(x64_RBP), MINV);
  EMIT(iface->output, X64_MOV, MREG(x64_RBP), MREG(x64_RSP), MINV);

  if (procedure->local_var_size > 0) {
    EMIT(iface->output, X64_SUB, MREG(x64_RSP), MIMM(((procedure->local_var_size / 16 + 1) * 16)), MINV);
  }

  for (int i = 0; i < procedure->param_count; i++) {
    EMIT(iface->output, X64_MOV, MSTK(((i + 1 ) * 4)), MREG(arg_registers_32bit[i]), MINV);
  }
}


void emit_x64_epilogue(CodegenInterface *iface, Procedure *procedure) {
  char name_end[256];
  snprintf(name_end, 256, ".end");
  
  EMIT(iface->output, X64_LABEL, MLBL(strdup(name_end)), MINV, MINV);
  
  if (procedure->local_var_size > 0) {
    EMIT(iface->output, X64_ADD, MREG(x64_RSP), MIMM(((procedure->local_var_size / 16 + 1) * 16)), MINV);
  }
  EMIT(iface->output, X64_POP, MINV, MREG(x64_RBP), MINV);
  EMIT(iface->output, X64_RET, MINV, MINV, MINV);
}


X64_INSTRUCTION(add) {
  EMIT(iface->output, X64_ADD, MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.dest.vreg].size, instr.dest.vreg)), MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.arg2.vreg].size, instr.arg2.vreg)), MINV);
}


X64_INSTRUCTION(sub) {
  EMIT(iface->output, X64_SUB, MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.dest.vreg].size, instr.dest.vreg)), MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.arg2.vreg].size, instr.arg2.vreg)), MINV);
}


X64_INSTRUCTION(mul) {
  EMIT(iface->output, X64_IMUL, MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.dest.vreg].size, instr.dest.vreg)), MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.arg2.vreg].size, instr.arg2.vreg)), MINV);
}


X64_INSTRUCTION(load) {
  EMIT(iface->output, X64_MOV, MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.dest.vreg].size, instr.dest.vreg)), MSTK(instr.arg1.stack_offset), MINV);
}

X64_INSTRUCTION(load_index) {
  EMIT(iface->output, X64_MOV, MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.dest.vreg].size, instr.dest.vreg)), MSTK(instr.arg1.stack_offset), MREG( VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.arg2.vreg].size, instr.arg2.vreg)));
}


X64_INSTRUCTION(store) {
  EMIT(iface->output, X64_MOV, MSTK(instr.dest.stack_offset), MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.arg1.vreg].size, instr.arg1.vreg)), MINV);
}


X64_INSTRUCTION(loadi) {
  EMIT(iface->output, X64_MOV, MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.dest.vreg].size, instr.dest.vreg)), MIMM(instr.arg1.imm), MINV);
}


X64_INSTRUCTION(copy) {
  if (VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.dest.vreg].size, instr.dest.vreg) == (VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.arg1.vreg].size, instr.arg1.vreg))) return;

  EMIT(iface->output, X64_MOV, MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.dest.vreg].size, instr.dest.vreg)), MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.arg1.vreg].size, instr.arg1.vreg)), MINV);
}


X64_INSTRUCTION(ret) {
  EMIT(iface->output, X64_MOV, MREG(x64_EAX), MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.arg1.vreg].size, instr.arg1.vreg)), MINV);
  EMIT(iface->output, X64_JMP, MLBL(".end"), MINV, MINV); // goes to epilogue (works for nasm)
}


X64_INSTRUCTION(jmp) {
  EMIT(iface->output, X64_JMP, MLBL(instr.dest.label_name), MINV, MINV);
}


X64_INSTRUCTION(jmpCC) {
  EMIT(iface->output, X64_CMP, MINV, MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.arg1.vreg].size, instr.arg1.vreg)), MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.arg2.vreg].size, instr.arg2.vreg)));
  EMIT(iface->output, jmpCC_from_ir[instr.opcode], MLBL(instr.dest.label_name), MINV, MINV);
}


X64_INSTRUCTION(cmpCC) {
  EMIT(iface->output, X64_CMP, MINV, MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.arg1.vreg].size, instr.arg1.vreg)), MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.arg2.vreg].size, instr.arg2.vreg)));
  EMIT(iface->output, setCC_from_ir[instr.opcode], MREG(VIRT_TO_PHYS_REG(8, instr.dest.vreg)), MINV, MINV);
}


X64_INSTRUCTION(call) {
  // preliminary spilling live registers to slots after local variables
  int spill_offset = 0;
  for (int j = 0; j < iface->interval.count; j++) {
      Interval span = iface->interval.items[j];
      if (span.start < iface->ctx.current_instruction && iface->ctx.current_instruction < span.end) {
          spill_offset += 4;
          EMIT(iface->output, X64_MOV, MSTK(spill_offset + iface->ctx.stack_offset), MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[span.vreg].size, span.vreg)), MINV);
      }
  }

  for (int j = 0; j < iface->ctx.arg_count; j++) {
    EMIT(iface->output, X64_MOV, MREG(arg_registers_32bit[j]), MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[iface->ctx.args[j].vreg].size, iface->ctx.args[j].vreg)), MINV);
  }
  
  iface->ctx.arg_count = 0;
  EMIT(iface->output, X64_CALL, MLBL(instr.arg1.label_name), MINV, MINV);
  
  if (instr.dest.type == OPERAND_VREG) {
    EMIT(iface->output, X64_MOV, MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[instr.dest.vreg].size, instr.dest.vreg)), MREG(x64_EAX), MINV);
  }
  spill_offset = 0;
  for (int j = 0; j < iface->interval.count; j++) {
      Interval span = iface->interval.items[j];
      if (span.start < iface->ctx.current_instruction && iface->ctx.current_instruction < span.end) {
          spill_offset += 4;
          EMIT(iface->output, X64_MOV, MREG(VIRT_TO_PHYS_REG(iface->vreg_info.items[span.vreg].size, span.vreg)), MSTK(spill_offset + iface->ctx.stack_offset), MINV);
      }
    }
}


X64_INSTRUCTION(label) {
  EMIT(iface->output, X64_LABEL, MLBL(instr.dest.label_name), MINV, MINV);
}


X64_INSTRUCTION(param) {
  iface->ctx.args[iface->ctx.arg_count++] = instr.arg1;
}


void emit_entry(FILE *out, Program *program) {
    fprintf(out, "section\t.text\n");
    fprintf(out, ENTRY_SYMBOL ":\n");
    fprintf(out, "\tcall\tmain\n");
    fprintf(out, "\tmov\tedi, eax\n");
    fprintf(out, "\tmov\teax, " EXIT_STATUS "\n");
    fprintf(out, "\tsyscall\n\n");
}


void emit_symbols(FILE *out, Program *program) {
    fprintf(out, "global\t" ENTRY_SYMBOL "\n");
    for (int i = 0; i < program->count; i++) {
      if (program->items[i].externed)
        fprintf(out, "extern\t%s\n", program->items[i].name);
    }
    fprintf(out, "\n");
}


CodegenBackend LINUX_X86_64_BACKEND = (CodegenBackend) {
  .adhere_bytecode_to_spec = &adhere_bytecode_to_machine_spec,
  .print_machine_code = &fprint_x64_machine_code,
  .emit_entry = &emit_entry,
  .emit_symbols = &emit_symbols,
  .emit_epilogue = &emit_x64_epilogue,
  .emit_prologue = &emit_x64_prologue,
  .emit_machine_code_from =  {
    [OPCODE_ADD] = &emit_x64_from_ir_add,
    [OPCODE_SUB] = &emit_x64_from_ir_sub,
    [OPCODE_MUL] = &emit_x64_from_ir_mul,
    [OPCODE_DIV] = NULL,//&emit_x64_from_ir_div,
    [OPCODE_CMP_EQ] = &emit_x64_from_ir_cmpCC,
    [OPCODE_CMP_LT] = &emit_x64_from_ir_cmpCC,
    [OPCODE_CMP_GT] =  &emit_x64_from_ir_cmpCC,
    [OPCODE_CMP_LEQ] = &emit_x64_from_ir_cmpCC,
    [OPCODE_CMP_GEQ] = &emit_x64_from_ir_cmpCC,
    [OPCODE_LOAD] = &emit_x64_from_ir_load,
    [OPCODE_LOAD_INDEX] = &emit_x64_from_ir_load_index,//&emit_x64_from_ir_,
    [OPCODE_STORE] = &emit_x64_from_ir_store,
    [OPCODE_COPY] = &emit_x64_from_ir_copy,
    [OPCODE_LOADI] = &emit_x64_from_ir_loadi,
    [OPCODE_RET] = &emit_x64_from_ir_ret,
    [OPCODE_JMP] = &emit_x64_from_ir_jmp,
    [OPCODE_JMP_EQ] = &emit_x64_from_ir_jmpCC,
    [OPCODE_JMP_NEQ] = &emit_x64_from_ir_jmpCC,
    [OPCODE_JMP_LT]  = &emit_x64_from_ir_jmpCC,
    [OPCODE_JMP_GT]  = &emit_x64_from_ir_jmpCC,
    [OPCODE_JMP_LE]  = &emit_x64_from_ir_jmpCC,
    [OPCODE_JMP_GE]  = &emit_x64_from_ir_jmpCC,
    [OPCODE_CALL] = &emit_x64_from_ir_call,
    [OPCODE_PARAM] = &emit_x64_from_ir_param,
    [OPCODE_LABEL] = &emit_x64_from_ir_label,
  },
};
