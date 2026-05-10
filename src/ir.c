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

const OpcodeDesc opcode_description_table[] = {
    [OPCODE_NONE]        = { OPDR_NONE, OPDR_NONE, OPDR_NONE, 0},
    [OPCODE_ADD]         = { OPDR_DEF,   OPDR_USE, OPDR_USE,  0},
    [OPCODE_SUB]         = { OPDR_DEF,   OPDR_USE, OPDR_USE,  OPC_NONCOMM},
    [OPCODE_MUL]         = { OPDR_DEF,   OPDR_USE, OPDR_USE,  0},
    [OPCODE_DIV]         = { OPDR_DEF,   OPDR_USE, OPDR_USE,  OPC_NONCOMM},
    [OPCODE_CMP_EQ]      = { OPDR_DEF,   OPDR_USE, OPDR_USE,  0},
    [OPCODE_CMP_LT]      = { OPDR_DEF,   OPDR_USE, OPDR_USE,  OPC_NONCOMM},
    [OPCODE_CMP_GT]      = { OPDR_DEF,   OPDR_USE, OPDR_USE,  OPC_NONCOMM},
    [OPCODE_CMP_LEQ]     = { OPDR_DEF,   OPDR_USE, OPDR_USE,  OPC_NONCOMM},
    [OPCODE_CMP_GEQ]     = { OPDR_DEF,   OPDR_USE, OPDR_USE,  OPC_NONCOMM},
    [OPCODE_LOAD]        = { OPDR_DEF,   OPDR_USE, OPDR_NONE, 0},
    [OPCODE_IMM]         = { OPDR_DEF,   OPDR_USE, OPDR_NONE, 0},
    [OPCODE_INDEX]       = { OPDR_DEF,   OPDR_USE, OPDR_NONE, 0},
    [OPCODE_ADDR]        = { OPDR_DEF,   OPDR_USE, OPDR_NONE, 0},
    [OPCODE_STORE]       = { OPDR_USE,   OPDR_USE, OPDR_NONE, 0},
    [OPCODE_STORE_INDEX] = { OPDR_USE,   OPDR_USE, OPDR_NONE, 0},
    [OPCODE_COPY]        = { OPDR_DEF,   OPDR_USE, OPDR_NONE, 0},
    [OPCODE_CALL]        = { OPDR_DEF,   OPDR_USE, OPDR_NONE, 0},
    [OPCODE_ARG]         = { OPDR_NONE,  OPDR_USE, OPDR_NONE, 0},
    [OPCODE_PARAM]       = { OPDR_DEF,   OPDR_USE, OPDR_NONE, 0},
    [OPCODE_JMP]         = { OPDR_USE,  OPDR_NONE, OPDR_NONE, 0},
    [OPCODE_BEQ]         = { OPDR_USE,   OPDR_USE, OPDR_USE,  0},
    [OPCODE_BLT]         = { OPDR_USE,   OPDR_USE, OPDR_USE,  0},
    [OPCODE_BGT]         = { OPDR_USE,   OPDR_USE, OPDR_USE,  0},
    [OPCODE_BGE]         = { OPDR_USE,   OPDR_USE, OPDR_USE,  0},
    [OPCODE_BLE]         = { OPDR_USE,   OPDR_USE, OPDR_USE,  0},
    [OPCODE_RET]         = { OPDR_NONE,  OPDR_USE, OPDR_NONE, 0},
};


void print_program(Program program, bool colored) {
    printf("Program:\n");
    for (int i = 0; i < program.count; i++) {
        if (!colored) print_procedure(program.items[i]);
        else print_procedure_colored(&program.items[i]);
    }
}


void print_procedure(Procedure procedure) {
    printf("%s:\n", procedure.name);
    print_cfg(procedure.cfg);
}


bool print_color_otherwise_operand(Operand operand, VregInfoTable *vregs, bool leading) {
    switch (operand.type) {
        case OPERAND_VREG:
            if (vregs->items[operand.vreg].color.index != -1) {
                if (leading) printf(", ");
                printf("c%d", vregs->items[operand.vreg].color.index);
                return true;
            }
            break;
        case OPERAND_MEM:
            printf("[");
            printf("c%d", vregs->items[operand.mem.base].color.index);
            if (operand.mem.stride > 0) {
                printf(" + %d * ", operand.mem.stride);
               printf("c%d", vregs->items[operand.mem.index].color.index);
            }
            if (operand.mem.offset > 0)
                printf(" - %d", abs(operand.mem.offset));
            else if (operand.mem.offset < 0)
                printf(" + %d", abs(operand.mem.offset));
            printf("]");
            break;
        default:
            return print_operand(operand, leading);
    }
    return false;
}


void print_procedure_colored(Procedure *procedure) {
    printf("%s colored:\n", procedure->name);

    for (int i = 0; i < procedure->cfg.count; i++) {
        int b = procedure->cfg.rpo_list[i];

        printf("b%d:\n", b);

        for (int j = 0; j < procedure->cfg.items[b].bytecode.count; j++) {
            Instr instr = procedure->cfg.items[b].bytecode.items[j];

            printf("\t");
            switch (instr.dest.type) {
                case OPERAND_NONE:
                case OPERAND_BLOCK:
                    printf("%s ", opcode_to_string[instr.opcode]);
                    print_color_otherwise_operand(instr.arg2, &procedure->vregs,
                        print_color_otherwise_operand(instr.arg1, &procedure->vregs,
                        print_color_otherwise_operand(instr.dest, &procedure->vregs, false)));
                    break;
                default:
                    print_color_otherwise_operand(instr.dest, &procedure->vregs, false);
                    printf(" = %s ", opcode_to_string[instr.opcode]);
                    print_color_otherwise_operand(instr.arg2, &procedure->vregs, print_color_otherwise_operand(instr.arg1, &procedure->vregs, false));
                    break;
            }
            printf("\n");
        }

        printf("s: ");
        for (int j = 0; j < 2; j++) {
            if (procedure->cfg.items[b].successors[j] != -1)
                printf("b%d ", procedure->cfg.items[b].successors[j]);
        }
        printf("\n");
    }
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

    print_block(graph, walked, b.successors[0]);
    print_block(graph, walked, b.successors[1]);
}


void print_mem(MemRef mem) {
    printf("["); printf("r%d", mem.base);
    if (mem.stride > 0) {
        printf(" + %d * ", mem.stride);
        printf("r%d", mem.index);
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
        case OPERAND_MEM: print_mem(operand.mem); break;
        case OPERAND_IMM: printf("%d", operand.imm); break;
        case OPERAND_SLOT: printf("s%d", operand.slot); break;
        case OPERAND_GLOBAL: printf("GBL%d", operand.glbl); break;
        default: return false;
    }
    return true;
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


// void print_vregs(Procedure procedure) {
//     for (int v = 0; v < procedure.vregs.count; v++) {
//         VregInfo entry = procedure.vregs.items[v];
//         // if (entry.color.index == -1) continue;
//         printf("{ r%d | color: %d, size: %d, signed: %d }\n", v, entry.color.index, entry.size, entry.sign);
//     }
// }

// if arg is -1, matches anything
inline bool instr_match(Instr *instr, Opcode opcode, OperandType dest, OperandType arg1, OperandType arg2) {
    return (instr->opcode == opcode &&
                (dest == -1 || instr->dest.type == dest) &&
                (arg1 == -1 || instr->arg1.type == arg1) &&
                (arg2 == -1 || instr->arg2.type == arg2));
}

bool operand_has_vreg(Operand operand, int vreg) {
    return (operand.type == OPERAND_VREG && operand.vreg == vreg) ||
            (operand.type == OPERAND_MEM && (operand.mem.base == vreg || operand.mem.index == vreg));
}

bool instr_defines_vreg(Instr instr, int vreg) {
    OpcodeDesc roles = opcode_description_table[instr.opcode];
    
    if (roles.dest == OPDR_DEF && operand_has_vreg(instr.dest, vreg)) return true;
    if (roles.arg1 == OPDR_DEF && operand_has_vreg(instr.arg1, vreg)) return true;
    if (roles.arg2 == OPDR_DEF && operand_has_vreg(instr.arg2, vreg)) return true;

    return false;
}

void operand_replace_vreg(Operand *operand, int find, int replace) {
    if (operand->type == OPERAND_VREG && operand->vreg == find) operand->vreg = replace;
    if (operand->type == OPERAND_MEM && operand->mem.index == find) operand->mem.index = replace;
    if (operand->type == OPERAND_MEM && operand->mem.base == find) operand->mem.base = replace;
}

void instr_replace_vreg(Instr *instr, int find, int replace) {
    operand_replace_vreg(&instr->dest, find, replace);
    operand_replace_vreg(&instr->arg1, find, replace);
    operand_replace_vreg(&instr->arg2, find, replace);
}

bool instr_uses_vreg(Instr instr, int vreg) {
    OpcodeDesc roles = opcode_description_table[instr.opcode];
    
    if (roles.dest == OPDR_USE && operand_has_vreg(instr.dest, vreg)) return true;
    if (roles.arg1 == OPDR_USE && operand_has_vreg(instr.arg1, vreg)) return true;
    if (roles.arg2 == OPDR_USE && operand_has_vreg(instr.arg2, vreg)) return true;

    return false;
}

int operand_collect_vregs(Operand opnd, int *out, int offset) {
    int size = 0;
    if (opnd.type == OPERAND_VREG) {
        out[offset + size++] = opnd.vreg;
    } else if (opnd.type == OPERAND_MEM) {
        if (opnd.mem.base != -1) out[offset + size++] = opnd.mem.base;
        if (opnd.mem.index != -1) out[offset + size++] = opnd.mem.index;
    }
    return size;
}

int instr_collect_used_vregs(Instr instr, int *out) {
    int size = 0;
    OpcodeDesc roles = opcode_description_table[instr.opcode];

    if (roles.dest == OPDR_USE) size += operand_collect_vregs(instr.dest, out, size);
    if (roles.arg1 == OPDR_USE) size += operand_collect_vregs(instr.arg1, out, size);
    if (roles.arg2 == OPDR_USE) size += operand_collect_vregs(instr.arg2, out, size);

    return size;
}

int instr_collect_used_slots(Instr instr, Operand *out[]) {
    int size = 0;
    OpcodeDesc roles = opcode_description_table[instr.opcode];

    if (roles.dest == OPDR_USE && instr.dest.type == OPERAND_SLOT ) out[size++] = &instr.dest;
    if (roles.arg1 == OPDR_USE && instr.arg1.type == OPERAND_SLOT ) out[size++] = &instr.arg1;
    if (roles.arg2 == OPDR_USE && instr.arg2.type == OPERAND_SLOT ) out[size++] = &instr.arg2;

    return size;
}

int instr_collect_def_slots(Instr instr, Operand *out[]) {
    int size = 0;
    OpcodeDesc roles = opcode_description_table[instr.opcode];

    if (roles.dest == OPDR_DEF && instr.dest.type == OPERAND_SLOT ) out[size++] = &instr.dest;
    if (roles.arg1 == OPDR_DEF && instr.arg1.type == OPERAND_SLOT ) out[size++] = &instr.arg1;
    if (roles.arg2 == OPDR_DEF && instr.arg2.type == OPERAND_SLOT ) out[size++] = &instr.arg2;

    return size;
}


// bool vreg_in_use(Instr instr, int vreg) {
//     return (instr.arg1.type == OPERAND_VREG && instr.arg1.vreg == vreg)
//            || (instr.arg2.type == OPERAND_VREG && instr.arg2.vreg == vreg)
//            || (instr.dest.type == OPERAND_MEM && instr.dest.mem.base.type == ADDR_VREG && instr.dest.mem.base.vreg == vreg)
//            || (instr.dest.type == OPERAND_MEM && instr.dest.mem.index.type == ADDR_VREG && instr.dest.mem.index.vreg == vreg);
// }


// int vreg_if_use(Instr instr, int *vregs) {
//     int size = 0;
//     if (instr.arg1.type == OPERAND_VREG) {
//         vregs[size++] = instr.arg1.vreg;
//     }
//     if (instr.arg2.type == OPERAND_VREG) {
//         vregs[size++] = instr.arg2.vreg;
//     }
//     if (instr.dest.type == OPERAND_MEM) {
//         if (instr.dest.mem.base.type == ADDR_VREG) {
//             vregs[size++] = instr.dest.mem.base.vreg;
//         }
//         if (instr.dest.mem.index.type == ADDR_VREG) {
//             vregs[size++] = instr.dest.mem.index.vreg;
//         }
//     }
//     return size;
// }


inline VregInfo vregi_from_sloti(SlotInfo slot) {
    return (VregInfo) {
        .size = slot.size,
        .sign = slot.sign
    };
}
