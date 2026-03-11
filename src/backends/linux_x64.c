    /*OPCODE_ADD,
    OPCODE_SUB,
    OPCODE_MUL,
    OPCODE_DIV,
    OPCODE_CMP_EQ,
    OPCODE_CMP_LT,
    OPCODE_CMP_GT,
    OPCODE_CMP_LEQ,
    OPCODE_CMP_GEQ,
    OPCODE_LOAD, // TODO: currently load and store operate on the stack.
               // will need to seperate stack loads from regular loads
    OPCODE_STORE,
    OPCODE_COPY,
    OPCODE_LOADI,
    OPCODE_RET,

    OPCODE_JMP,
    OPCODE_JMP_EQ,
    OPCODE_JMP_NEQ,
    OPCODE_CALL,

    OPCODE_LABEL,      // TODO: might not be the best to conflict metadata on bytecode with operands. this is just for time being
    OPCODE_PROC_BEGIN, // dest is proc name, arg1 is max stack offset
    OPCODE_PROC_END,   // dest is proc name, arg1 is max stack offset*/

void emit_x64_add(MachCode *mach_code, Instr *instr) {
    MachOp op;
    op.opcode = X64_MOV;
    op.dest.type = MACH_VREG;
    op.dest.vreg = instr->arg1.vreg;
    array_append(*mach_code, op);
}
void emit_x64_sub(MachCode *mach_code, Instr *instr) {
}
void emit_x64_imul(MachCode *mach_code, Instr *instr) {
}
void emit_x64_div(MachCode *mach_code, Instr *instr) {
}

// TODO: trying to completely change how the backend transformation from IR into machine code works.
// This function is WIP
MachCode instruction_selection(Bytecode bytecode, PhysRegs *pregs) {
    MachCode mach_code = {0};
    for (int i = 0; i < bytecode.count; i++) {
        Instr *instr = &bytecode.items[i];
        switch (instr->opcode) {
            case OPCODE_ADD:
                emit_x64_add(&mach_code, instr);
                break;
            case OPCODE_SUB:
                emit_x64_sub(&mach_code, instr);
                break;
            case OPCODE_MUL:
                emit_x64_imul(&mach_code, instr);
                break;
            case OPCODE_DIV:
                emit_x64_div(&mach_code, instr);
                break;
            default:
                UNREACHABLE("Opcode");
        }
    }
    return mach_code;
}

