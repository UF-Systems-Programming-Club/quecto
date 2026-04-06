#include "backends/linux_x64.h"
#include "codegen.h"
  
#define CONCAT_HELPER(a, b) a##b
#define CONCAT(a, b) CONCAT_HELPER(a, b)

#define STR_HELPER(X) #X
#define STR(X) STR_HELPER(X)

#define REG_NAME(SIZE) CONCAT(registers_, CONCAT(SIZE, bit))
#define VIRT_TO_PHYS_REG(SIZE, ARG) REG_NAME(SIZE)[iface->location.items[(ARG)].register_index]
#define X64_INSTRUCTION(name) void emit_x64_from_ir_##name(CodegenInterface *iface, Instr instr)

x64_Register registers_64bit[4] = {
  x64_RAX, x64_RCX, x64_RDX, x64_RDI
};


x64_Register registers_32bit[4] = {
  x64_EAX, x64_ECX, x64_EDX, x64_EDI
};


x64_Register registers_8bit[4] = {
  x64_AL, x64_CL, x64_DL, x64_DIL
};


x64_Register arg_registers_32bit[4] = {
  x64_EDI, x64_ESI, x64_EDX, x64_ECX
};


X64_INSTRUCTION(add) {
  MachineInstr code = {0};
  code.instruction = X64_ADD;
  
  code.dest = (MachineOperand) {
    .type = MOPERAND_REG,
    .reg = VIRT_TO_PHYS_REG(32, instr.dest.vreg),
  };
  
  code.arg1 = (MachineOperand) {
    .type = MOPERAND_REG,
    .reg = VIRT_TO_PHYS_REG(32, instr.arg2.vreg),
  };
  
  array_append(iface->output, code);
}


X64_INSTRUCTION(sub) {
  MachineInstr code = {0};
  code.instruction = X64_SUB;
  
  code.dest = (MachineOperand) {
    .type = MOPERAND_REG,
    .reg = VIRT_TO_PHYS_REG(32, instr.dest.vreg),
  };
  
  code.arg1 = (MachineOperand) {
    .type = MOPERAND_REG,
    .reg = VIRT_TO_PHYS_REG(32, instr.arg2.vreg),
  };
  
  array_append(iface->output, code);
}


X64_INSTRUCTION(mul) {
  MachineInstr code = {0};
  code.instruction = X64_IMUL;
  
  code.dest = (MachineOperand) {
    .type = MOPERAND_REG,
    .reg = VIRT_TO_PHYS_REG(32, instr.dest.vreg),
  };
  
  code.arg1 = (MachineOperand) {
    .type = MOPERAND_REG,
    .reg = VIRT_TO_PHYS_REG(32, instr.arg2.vreg),
  };
  
  array_append(iface->output, code);
}



X64_INSTRUCTION(cmp) {
  MachineInstr code = {0};
  code.instruction = X64_CMP;
  
  code.arg1 = (MachineOperand) {
    .type = MOPERAND_REG,
    .reg = VIRT_TO_PHYS_REG(32, instr.arg1.vreg),
  };
  
  code.arg2 = (MachineOperand) {
    .type = MOPERAND_REG,
    .reg = VIRT_TO_PHYS_REG(32, instr.arg2.vreg),
  };
  
  array_append(iface->output, code);
}


void emit_stack_load(MachineCode *output, x64_Register dst, int offset) {
  MachineInstr code = {0};
  code.instruction = X64_MOV;
  
  code.dest = (MachineOperand) {
    .type = MOPERAND_REG,
    .reg = dst,
  };
  
  code.arg1 = (MachineOperand) {
    .type = MOPERAND_STACK,
    .stack = offset,
  };

  array_append(*output, code);
}


void emit_stack_store(MachineCode *output, int dst, x64_Register reg) {
  MachineInstr code = {0};
  code.instruction = X64_MOV;
  
  code.dest = (MachineOperand) {
    .type = MOPERAND_STACK,
    .reg = dst,
  };
  
  code.arg1 = (MachineOperand) {
    .type = MOPERAND_REG,
    .stack = reg,
  };

  array_append(*output, code);
}


X64_INSTRUCTION(load) {
  emit_stack_load(&iface->output, VIRT_TO_PHYS_REG(32, instr.dest.vreg), instr.arg1.stack_offset);
}


X64_INSTRUCTION(store) {
  emit_stack_store(&iface->output, instr.dest.stack_offset, VIRT_TO_PHYS_REG(32, instr.arg1.vreg));
}

X64_INSTRUCTION(loadi) {
  MachineInstr code = {0};
  code.instruction = X64_MOV;

  code.dest = (MachineOperand) {
    .type = MOPERAND_REG,
    .reg = VIRT_TO_PHYS_REG(32, instr.dest.vreg),
  };

  code.arg1 = (MachineOperand) {
    .type = MOPERAND_IMM,
    .imm = instr.arg1.imm,
  };

  array_append(iface->output, code);
}


void emit_copy(MachineCode *output, x64_Register dst, x64_Register src) {
  MachineInstr code = {0};
  code.instruction = X64_MOV;

  code.dest = (MachineOperand) {
    .type = MOPERAND_REG,
    .imm = dst,
  };

  code.arg1 = (MachineOperand) {
    .type = MOPERAND_REG,
    .imm = src,
  };

  array_append(*output, code);
}


X64_INSTRUCTION(copy) {
  if (VIRT_TO_PHYS_REG(32, instr.dest.vreg) == (VIRT_TO_PHYS_REG(32, instr.arg1.vreg))) return;

  emit_copy(&iface->output,
            VIRT_TO_PHYS_REG(32, instr.dest.vreg),
            VIRT_TO_PHYS_REG(32, instr.arg1.vreg)
  );
}


void emit_jump(MachineCode *output, X64_Opcode type,  const char *label) {
  MachineInstr code = {0};
  code.instruction = type;

  code.dest = (MachineOperand) {
    .type = MOPERAND_LABEL,
    .label = label,
  };

  array_append(*output, code);
}


X64_INSTRUCTION(ret) {
  emit_copy(&iface->output, x64_EAX, VIRT_TO_PHYS_REG(32, instr.arg1.vreg));
  
  // I really don't like this... it will be fixed later as control flow semantics change
  // ideally will not be flat represenation of labels but graph flow.

  char to[128];
  snprintf(to, 128, ".%s.end", iface->ctx.procedure_name);
  char *label = strdup(to);

  emit_jump(&iface->output, X64_JMP, label);
}


X64_INSTRUCTION(jmp) {
  emit_jump(&iface->output, X64_JMP, instr.dest.label_name);
}


X64_INSTRUCTION(jmp_eq) {
  // to be added  
}


X64_INSTRUCTION(jmp_neq) {
  MachineInstr code = {0};
  code.instruction = X64_CMP;

  code.arg1 = (MachineOperand) {
    .type = MOPERAND_REG,
    .reg = VIRT_TO_PHYS_REG(32, instr.arg1.vreg),
  };

  code.arg2 = (MachineOperand) {
    .type = MOPERAND_IMM,
    .imm = 1,
  };
  
  array_append(iface->output, code);
}


X64_INSTRUCTION(call) {
  // preliminary spilling live registers to slots after local variables
  int spill_offset = 0;
  for (int j = 0; j < iface->interval.count; j++) {
      Interval span = iface->interval.items[j];
      if (span.start < iface->ctx.current_instruction && iface->ctx.current_instruction < span.end) {
          spill_offset += 4;
          emit_stack_store(&iface->output,
                           spill_offset + iface->ctx.stack_offset,
                            VIRT_TO_PHYS_REG(32, span.vreg)
                          );
      }
  }

  for (int j = 0; j < iface->ctx.arg_count; j++) {
    emit_copy(&iface->output,
              arg_registers_32bit[j],
              VIRT_TO_PHYS_REG(32, iface->ctx.args[j].vreg)
            );
  }
  
  iface->ctx.arg_count = 0;
  emit_jump(&iface->output, X64_CALL, instr.arg1.label_name);
  
  if (instr.dest.type == OPERAND_VREG)
      emit_copy(&iface->output, VIRT_TO_PHYS_REG(32, instr.dest.vreg), x64_EAX);
    
  spill_offset = 0;
  for (int j = 0; j < iface->interval.count; j++) {
      Interval span = iface->interval.items[j];
      if (span.start < iface->ctx.current_instruction && iface->ctx.current_instruction < span.end) {
          spill_offset += 4;
          emit_stack_load(&iface->output,
                          VIRT_TO_PHYS_REG(32, span.vreg),
                          spill_offset + iface->ctx.stack_offset);
      }
    }
}


X64_INSTRUCTION(label) {
  MachineInstr code = {0};
  code.instruction = X64_LABEL;

  code.dest = (MachineOperand) {
    .type = MOPERAND_LABEL,
    .label = instr.dest.label_name,
  };

  array_append(iface->output, code);
}

X64_INSTRUCTION(param) {
  iface->ctx.args[iface->ctx.arg_count++] = instr.arg1;
}

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
};

bool print_machine_operand(MachineOperand operand) {
  switch (operand.type) {
    case MOPERAND_IMM:
      printf("%d", operand.imm);
      break;
    case MOPERAND_REG:
      printf("%s", x86_reg_to_str[operand.reg]);
      break;
    case MOPERAND_STACK:
      printf("[rbp - %d]", operand.stack);
      break;
    case MOPERAND_LABEL:
      printf("%s", operand.label);
      break;
    case MOPERAND_INVALID:
      return false;
  }
  return true;
}

void print_x64_machine_code(MachineCode *code) {
  for (int i = 0; i < code->count; i++) {
    MachineInstr instr = code->items[i];
    switch (instr.instruction) {
      case X64_LABEL:
        printf("%s\n", instr.dest.label);
        break;
      default:
        printf("\t%s\t", x86_op_to_str[instr.instruction]);
        print_machine_operand(instr.dest);
        if (instr.arg1.type != MOPERAND_INVALID) {
          printf(", ");
          print_machine_operand(instr.arg1);
        }
        if (instr.arg2.type != MOPERAND_INVALID) {
          printf(", ");
          print_machine_operand(instr.arg2);
        }
        printf("\n");
      break;
    }
  }
}

CodegenBackend LINUX_X86_64_BACKEND = (CodegenBackend) {
  .adhere_bytecode_to_spec = &adhere_bytecode_to_machine_spec,
  .print_machine_code = &print_x64_machine_code,
  .emit_machine_code_from =  {
    [OPCODE_ADD] = &emit_x64_from_ir_add,
    [OPCODE_SUB] = &emit_x64_from_ir_sub,
    [OPCODE_MUL] = &emit_x64_from_ir_mul,
    [OPCODE_DIV] = NULL,//&emit_x64_from_ir_div,
    [OPCODE_CMP] = &emit_x64_from_ir_cmp,
    [OPCODE_CMP_EQ] = NULL,
    [OPCODE_CMP_LT] = NULL,
    [OPCODE_CMP_GT] = NULL,
    [OPCODE_CMP_LEQ] = NULL,
    [OPCODE_CMP_GEQ] = NULL,
    [OPCODE_LOAD] = &emit_x64_from_ir_load,
    [OPCODE_LOAD_INDEX] = NULL, //&emit_x64_from_ir_,
    [OPCODE_STORE] = &emit_x64_from_ir_store,
    [OPCODE_COPY] = &emit_x64_from_ir_copy,
    [OPCODE_LOADI] = &emit_x64_from_ir_loadi,
    [OPCODE_RET] = &emit_x64_from_ir_ret,
    [OPCODE_JMP] = &emit_x64_from_ir_jmp,
    [OPCODE_JMP_NEQ] = &emit_x64_from_ir_jmp_neq,
    [OPCODE_CALL] = &emit_x64_from_ir_call,
    [OPCODE_PARAM] = &emit_x64_from_ir_param,
    [OPCODE_LABEL] = &emit_x64_from_ir_label,
  },
};

// // TODO: trying to completely change how the backend transformation from IR into machine code works.
// // This function is WIP
// MachCode instruction_selection(Bytecode bytecode, PhysRegs *pregs) {
//     MachCode mach_code = {0};
//     for (int i = 0; i < bytecode.count; i++) {
//         Instr *instr = &bytecode.items[i];
//         switch (instr->opcode) {
//             case OPCODE_ADD:
//                 emit_x64_add(&mach_code, instr);
//                 break;
//             case OPCODE_SUB:
//                 emit_x64_sub(&mach_code, instr);
//                 break;
//             case OPCODE_MUL:
//                 emit_x64_imul(&mach_code, instr);
//                 break;
//             case OPCODE_DIV:
//                 emit_x64_div(&mach_code, instr);
//                 break;
//             default:
//                 UNREACHABLE("Opcode");
//         }
//     }
//     return mach_code;
// }

