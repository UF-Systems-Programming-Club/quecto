#include <stdio.h>

#include "ast.h"
#include "common.h"
#include "ir.h"
#include "symbol_table.h"


const char *opcode_to_string[OPCODE_COUNT] = {
    [OPCODE_ADD]     = "add",
    [OPCODE_SUB]     = "sub",
    [OPCODE_MUL]     = "mul",
    [OPCODE_DIV]     = "div",
    [OPCODE_CMP_EQ]  = "ceq",
    [OPCODE_CMP_LT]  = "clt",
    [OPCODE_CMP_GT]  = "cgt",
    [OPCODE_CMP_LEQ] = "cle",
    [OPCODE_CMP_GEQ] = "cge",
    [OPCODE_LOAD]    = "load",
    [OPCODE_IMM]     = "imm",
    [OPCODE_INDEX]   = "index",
    [OPCODE_ADDR]    = "addr",
    [OPCODE_STORE]   = "store",
    [OPCODE_COPY]    = "copy",
    [OPCODE_RET]     = "ret",
    [OPCODE_JMP]     = "jmp",
    [OPCODE_BEQ]  = "beq",
    [OPCODE_BLT]  = "blt",
    [OPCODE_BGT]  = "bgt",
    [OPCODE_BGE]  = "bge",
    [OPCODE_BLE]  = "ble",
    [OPCODE_BNE] = "bne",
    [OPCODE_CALL]    = "call",
    [OPCODE_PARAM]   = "param",
    [OPCODE_ARG] = "arg",
};

const Opcode opposite_opcode_table[OPCODE_COUNT] = {
    [OPCODE_JMP] = OPCODE_JMP,
    [OPCODE_BNE] = OPCODE_BEQ,
    [OPCODE_BEQ] = OPCODE_BNE,
    [OPCODE_BGE] = OPCODE_BLT,
    [OPCODE_BLE] = OPCODE_BGT,
    [OPCODE_BGT] = OPCODE_BLE,
    [OPCODE_BLT] = OPCODE_BGE,
};


void print_program(Program program) {
    printf("Program:\n");
    for (int i = 0; i < program.count; i++) {
        print_procedure(program.items[i]);
    }
}

void print_procedure(Procedure procedure) {
    printf("%s:\n", procedure.name);
    // print_vregs(procedure);
    print_cfg(procedure.cfg);
}


void print_cfg(CFGraph graph) {
    bool *walked = calloc(graph.count, sizeof(bool));
    print_block(graph, walked, graph.entry_block);
    free(walked);
}


void print_block(CFGraph graph, bool walked[], int block) {
    if (block == -1 || walked[block]) return;

    
    BasicBlock b =  graph.items[block];
    
    printf("block #%d:\n", block);
    walked[block] = true;

    for (int i = 0; i < b.phis.count; i ++) {
        printf("\tr%d = phi(", b.phis.items[i].dest.vreg);
        for (int j = 0; j < b.predecessors.count; j++) {
            if (b.phis.items[i].args[j].type == OPERAND_VREG) printf("r%d", b.phis.items[i].args[j].vreg);
            if (j != b.predecessors.count - 1) printf(", ");
        }
        printf(")\n");
    }

    for (int i = 0; i < b.bytecode.count; i++) {
        printf("\t");print_instruction(b.bytecode.items[i]); printf("\n");
    }

    printf("\t");print_terminator(b.terminator);printf("\n");
    
    print_block(graph, walked, b.terminator.successor[0]);
    print_block(graph, walked, b.terminator.successor[1]);
}


void print_addr(Addr addr) {
    switch (addr.type) {
        case ADDR_VREG: printf("r%d", addr.vreg); break;
        case ADDR_REG: /*printf("%s", reg_str[addr.reg]);*/ break;
        case ADDR_NONE: break;
    }
}


void print_mem(MemRef mem) {
    printf("["); print_addr(mem.base);
    if (mem.stride > 0) {
        printf(" + %d * ", mem.stride);
        print_addr(mem.index);
    }
    if (mem.offset > 0) printf(" - %d", abs(mem.offset));
    else if (mem.offset < 0) printf(" + %d", abs(mem.offset));
    printf("]");
}


bool print_operand(Operand operand, bool leading) {
    if (operand.type != OPERAND_NONE && leading) printf(", ");
    switch (operand.type) {
        case OPERAND_BLOCK: printf("b%d", operand.block); break;
        case OPERAND_VREG: printf("r%d", operand.vreg); break;
        case OPERAND_REG: /*printf("%s", reg_str[operand.reg]);*/ break;
        case OPERAND_MEM: print_mem(operand.mem); break;
        case OPERAND_IMM: printf("%d", operand.imm); break;
        case OPERAND_SLOT: printf("s%d", operand.slot); break;
        case OPERAND_GLOBAL: printf("GBL%d", operand.glbl); break;
        default: return false;
    }
    return true;
}


void print_terminator(Instr instr) {
    switch (instr.opcode) {
        case OPCODE_JMP:
            instr.dest = (Operand) { .type = OPERAND_BLOCK, .block = instr.successor[0] };
            print_instruction(instr);
            break;
        case OPCODE_BEQ:
        case OPCODE_BNE:
        case OPCODE_BLT:
        case OPCODE_BGT:
        case OPCODE_BLE:
        case OPCODE_BGE:
            printf("%s (b%d|b%d) ", opcode_to_string[instr.opcode], instr.successor[0], instr.successor[1]);
            print_operand(instr.arg2, print_operand(instr.arg1, false));
            break;
        case OPCODE_RET:
            print_instruction(instr);
            break;
        default:
            assert(0 && "not proper opcode for terminator");
    }
}


void print_instruction(Instr instr) {
    switch (instr.dest.type) {
        case OPERAND_NONE:
        case OPERAND_BLOCK:
            printf("%s ", opcode_to_string[instr.opcode]);
            print_operand(instr.arg2, print_operand(instr.arg1, print_operand(instr.dest, false)));
            break;
        default:
            print_operand(instr.dest, false);
            printf(" = %s ", opcode_to_string[instr.opcode]);
            print_operand(instr.arg2, print_operand(instr.arg1, false));
            break;
    }
}


void print_bytecode(Bytecode bytecode, size_t bsize, int blocks[bsize]) {
    for (int i = 0; i < bytecode.count; i++) {
        for (int j = 0; j < bsize; j++)
            if (blocks[j] == i) printf("b%d: ", j);
        print_instruction(bytecode.items[i]);
    }
}


void print_vregs(Procedure procedure) {
    for (int v = 0; v < procedure.vregs.count; v++) {
        VregInfo entry = procedure.vregs.items[v];
        if (entry.color.index == -1) continue;
        printf("{ r%d | color: %d, size: %d, signed: %d }\n", v, entry.color.index, entry.size, entry.sign);
    }
}

// if arg is -1, matches anything
inline bool instr_match(Instr *instr, Opcode opcode, OperandType dest, OperandType arg1, OperandType arg2) {
    return (instr->opcode == opcode &&
                (dest == -1 || instr->dest.type == dest) &&
                (arg1 == -1 || instr->arg1.type == arg1) &&
                (arg2 == -1 || instr->arg2.type == arg2));
}


bool vreg_defined(Instr instr, int vreg) {
    return (instr.dest.type == OPERAND_VREG && instr.dest.vreg == vreg);
}


bool vreg_in_use(Instr instr, int vreg) {
    return (instr.arg1.type == OPERAND_VREG && instr.arg1.vreg == vreg)
           || (instr.arg2.type == OPERAND_VREG && instr.arg2.vreg == vreg)
           || (instr.dest.type == OPERAND_MEM && instr.dest.mem.base.type == ADDR_VREG && instr.dest.mem.base.vreg == vreg)
           || (instr.dest.type == OPERAND_MEM && instr.dest.mem.index.type == ADDR_VREG && instr.dest.mem.index.vreg == vreg);
}


int vreg_if_use(Instr instr, int *vregs) {
    int size = 0;
    if (instr.arg1.type == OPERAND_VREG) {
        vregs[size++] = instr.arg1.vreg;
    }
    if (instr.arg2.type == OPERAND_VREG) {
        vregs[size++] = instr.arg2.vreg;
    }
    if (instr.dest.type == OPERAND_MEM) {
        if (instr.dest.mem.base.type == ADDR_VREG) {
            vregs[size++] = instr.dest.mem.base.vreg;
        }
        if (instr.dest.mem.index.type == ADDR_VREG) {
            vregs[size++] = instr.dest.mem.index.vreg;
        }
    }
    return size;
}


inline VregInfo vregi_from_sloti(SlotInfo slot) {
    return (VregInfo) {
        .size = slot.size,
        .sign = slot.sign
    };
}
