#include <stdio.h>

#include "ast.h"
#include "common.h"
#include "ir.h"
#include "cfg.h"
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
    [OPCODE_EXT_Z]    = "extz",
    [OPCODE_IMM]     = "imm",
    [OPCODE_INDEX]   = "index",
    [OPCODE_ADDR]    = "addr",
    [OPCODE_STORE]   = "store",
    [OPCODE_STORE_INDEX]   = "store_idx",
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

#define BINARY_ASSIGN_DESC(op_flags) (OpcodeDesc){ \
    .dest = OPDR_DEF,\
    .arg1 = OPDR_USE,\
    .arg2 = OPDR_USE,\
    .flags =(op_flags),\
    .print_type = OP_PRINT_ASSIGN\
}

#define UNARY_ASSIGN_DESC(op_flags) (OpcodeDesc) {\
    .dest = OPDR_DEF,\
    .arg1 = OPDR_USE,\
    .arg2 = OPDR_NONE,\
    .flags =(op_flags),\
    .print_type = OP_PRINT_ASSIGN\
}

#define UNARY_STORE_DESC(op_flags) (OpcodeDesc) {\
    .dest = OPDR_USE,\
    .arg1 = OPDR_USE,\
    .arg2 = OPDR_NONE,\
    .flags = (op_flags),\
    .print_type = OP_PRINT_ASSIGN\
}

#define MARK_DESC(op_flags) (OpcodeDesc) {\
    .dest = OPDR_NONE,\
    .arg1 = OPDR_USE,\
    .arg2 = OPDR_NONE,\
    .flags = (op_flags),\
    .print_type = OP_PRINT_FUNC\
}

#define JUMP_DESC(op_flags) (OpcodeDesc) {\
    .dest = OPDR_USE,\
    .arg1 = OPDR_NONE,\
    .arg2 = OPDR_NONE,\
    .flags = (op_flags),\
    .print_type = OP_PRINT_BRANCH\
}

#define BRANCH_DESC(op_flags) (OpcodeDesc) {\
    .dest = OPDR_USE,\
    .arg1 = OPDR_USE,\
    .arg2 = OPDR_USE,\
    .flags = (op_flags),\
    .print_type = OP_PRINT_BRANCH\
}

const OpcodeDesc opcode_description_table[] = {
    [OPCODE_NONE]        = { 0 },
    [OPCODE_ADD]         = BINARY_ASSIGN_DESC(0),
    [OPCODE_SUB]         = BINARY_ASSIGN_DESC(OPC_NONCOMM),
    [OPCODE_MUL]         = BINARY_ASSIGN_DESC(0),
    [OPCODE_DIV]         = BINARY_ASSIGN_DESC(OPC_NONCOMM),
    [OPCODE_CMP_EQ]      = BINARY_ASSIGN_DESC(0),
    [OPCODE_CMP_LT]      = BINARY_ASSIGN_DESC(OPC_NONCOMM),
    [OPCODE_CMP_GT]      = BINARY_ASSIGN_DESC(OPC_NONCOMM),
    [OPCODE_CMP_LEQ]     = BINARY_ASSIGN_DESC(OPC_NONCOMM),
    [OPCODE_CMP_GEQ]     = BINARY_ASSIGN_DESC(OPC_NONCOMM),
    [OPCODE_LOAD]        = UNARY_ASSIGN_DESC(0),
    [OPCODE_EXT_Z]       = UNARY_ASSIGN_DESC(0),
    [OPCODE_IMM]         = UNARY_ASSIGN_DESC(0),
    [OPCODE_INDEX]       = UNARY_ASSIGN_DESC(0),
    [OPCODE_ADDR]        = UNARY_ASSIGN_DESC(0),
    [OPCODE_STORE]       = UNARY_STORE_DESC(0),
    [OPCODE_STORE_INDEX] = UNARY_STORE_DESC(0),
    [OPCODE_COPY]        = UNARY_ASSIGN_DESC(0),
    [OPCODE_CALL]        = UNARY_ASSIGN_DESC(0),
    [OPCODE_ARG]         = MARK_DESC(0),
    [OPCODE_PARAM]       = UNARY_ASSIGN_DESC(0),
    [OPCODE_JMP]         = JUMP_DESC(0),
    [OPCODE_BEQ]         = BRANCH_DESC(0),
    [OPCODE_BLT]         = BRANCH_DESC(0),
    [OPCODE_BGT]         = BRANCH_DESC(0),
    [OPCODE_BGE]         = BRANCH_DESC(0),
    [OPCODE_BLE]         = BRANCH_DESC(0),
    [OPCODE_BNE]         = BRANCH_DESC(0),
    [OPCODE_RET]         = MARK_DESC(0),
};


void ir_build_str_register(StringBuilder *sb, const Procedure *proc, size_t vreg) {
    (void)proc;
    string_builder_append_char(sb, 'r');
    string_builder_append_size_t(sb, vreg);
}


void ir_build_str_register_colored(StringBuilder *sb, const Procedure *proc, size_t vreg) {
    int color = proc->vregs.items[vreg].color.index;
    if (color == -1) {
        ir_build_str_register(sb, proc, vreg);
        return;
    }
    string_builder_append_char(sb, 'c');
    string_builder_append_size_t(sb, (size_t)color);
}


void ir_build_str_operand(StringBuilder *sb, const Procedure *proc, Operand operand, RegisterPrintFn *print_register) {
    switch (operand.type) {
        case OPERAND_BLOCK:
            string_builder_append_char(sb, 'b');
            string_builder_append_size_t(sb, operand.block);
            break;
        case OPERAND_VREG:
            print_register(sb, proc, operand.vreg);
            break;
        case OPERAND_MEM:
            string_builder_append_char(sb, '[');
            print_register(sb, proc, (size_t)operand.mem.base);
            if (operand.mem.stride > 0) {
                string_builder_sv_append(sb, SV_LITERAL(" + "));
                string_builder_append_size_t(sb, operand.mem.stride);
                string_builder_sv_append(sb, SV_LITERAL(" * "));
                print_register(sb, proc, (size_t)operand.mem.index);
            }
            if (operand.mem.offset != 0) {
                unsigned magnitude = (unsigned)operand.mem.offset;
                if (operand.mem.offset > 0) {
                    string_builder_sv_append(sb, SV_LITERAL(" - "));
                } else {
                    magnitude = 0u - magnitude;
                    string_builder_sv_append(sb, SV_LITERAL(" + "));
                }
                string_builder_append_size_t(sb, magnitude);
            }
            string_builder_append_char(sb, ']');
            break;
        case OPERAND_IMM:
            string_builder_append_size_t(sb, operand.imm);
            break;
        case OPERAND_SLOT:
            string_builder_append_char(sb, 's');
            string_builder_append_size_t(sb, operand.slot);
            break;
        case OPERAND_GLOBAL:
            string_builder_sv_append(sb, SV_LITERAL("GBL"));
            string_builder_append_size_t(sb, operand.glbl);
            break;
        default:
            break;
    }
}


void ir_build_str_phi(StringBuilder *sb, const Procedure *proc, Phi phi, size_t num_args, RegisterPrintFn *print_register) {
    ir_build_str_operand(sb, proc, phi.dest, print_register);
    string_builder_sv_append(sb, SV_LITERAL(" = phi "));
    for (size_t i = 0; i < num_args; i++) {
        ir_build_str_operand(sb, proc, phi.args[i], print_register);
        if (i + 1 < num_args)
            string_builder_sv_append(sb, SV_LITERAL(", "));
    }
}


void ir_build_str_instr(StringBuilder *sb, const Procedure *proc, Instr instr, RegisterPrintFn *print_register) {
    OpcodePrintType style = opcode_description_table[instr.opcode].print_type;

    switch (style) {
        case OP_PRINT_NONE:
            break;
        case OP_PRINT_ASSIGN:
            if (instr.dest.type != OPERAND_NONE) {
                ir_build_str_operand(sb, proc, instr.dest, print_register);
                string_builder_sv_append(sb, SV_LITERAL(" = "));
            }
            // fallthrough
        case OP_PRINT_FUNC:
            string_builder_cstr_append(sb, opcode_to_string[instr.opcode]);
            string_builder_append_char(sb, ' ');
            ir_build_str_operand(sb, proc, instr.arg1, print_register);
            if (instr.arg2.type != OPERAND_NONE) {
                string_builder_sv_append(sb, SV_LITERAL(", "));
                ir_build_str_operand(sb, proc, instr.arg2, print_register);
            }
            break;
        case OP_PRINT_BRANCH:
            string_builder_cstr_append(sb, opcode_to_string[instr.opcode]);
            string_builder_append_char(sb, ' ');
            if (instr.arg1.type != OPERAND_NONE) {
                ir_build_str_operand(sb, proc, instr.arg1, print_register);
                if (instr.arg2.type != OPERAND_NONE)
                    string_builder_sv_append(sb, SV_LITERAL(", "));
            }
            if (instr.arg2.type != OPERAND_NONE) {
                ir_build_str_operand(sb, proc, instr.arg2, print_register);
                if (instr.dest.type != OPERAND_NONE)
                    string_builder_sv_append(sb, SV_LITERAL(", "));
            }
            ir_build_str_operand(sb, proc, instr.dest, print_register);
            break;
    }
}

void ir_build_str_procedure(StringBuilder *sb, Arena *scratch, const Procedure *procedure, RegisterPrintFn *print_register) {
    CFGraph cfg = procedure->cfg;

    string_builder_cstr_append(sb, procedure->name);
    string_builder_sv_append(sb, SV_LITERAL(":\n"));

    if (cfg.count != 0 && cfg.rpo_list == NULL) {
        cfg.rpo = arena_alloc(scratch, cfg.count * sizeof *cfg.rpo);
        cfg.rpo_list = arena_alloc(scratch, cfg.count * sizeof *cfg.rpo_list);
        memset(cfg.rpo, -1, cfg.count * sizeof *cfg.rpo);
        memset(cfg.rpo_list, -1, cfg.count * sizeof *cfg.rpo_list);
        int k = 0;
        fill_rpo(&cfg, &k, (int)cfg.entry_block);
    }

    for (size_t l = 0; l < cfg.count; l++) {
        int b = cfg.rpo_list[l];
        if (b == -1) continue;

        string_builder_sv_append(sb, SV_LITERAL("\tblock "));
        string_builder_append_size_t(sb, b);
        string_builder_sv_append(sb, SV_LITERAL(":\n"));

        for (size_t m = 0; m < cfg.items[b].phis.count; m++) {
            string_builder_sv_append(sb, SV_LITERAL("\t\t"));
            ir_build_str_phi(sb, procedure, cfg.items[b].phis.items[m],
                             cfg.items[b].predecessors.count, print_register);
            string_builder_append_char(sb, '\n');
        }

        for (size_t m = 0; m < cfg.items[b].bytecode.count; m++) {
            string_builder_sv_append(sb, SV_LITERAL("\t\t"));
            ir_build_str_instr(sb, procedure, cfg.items[b].bytecode.items[m], print_register);
            string_builder_append_char(sb, '\n');
        }

        if (cfg.items[b].successors[0] == -1 && cfg.items[b].successors[1] == -1)
            continue;

        string_builder_sv_append(sb, SV_LITERAL("\t\tsuccessors: "));
        bool succ_printed = false;
        for (size_t m = 0; m < 2; m++) {
            int s = cfg.items[b].successors[m];
            if (s != -1) {
                if (succ_printed)
                    string_builder_sv_append(sb, SV_LITERAL(", "));
                string_builder_append_size_t(sb, s);
                succ_printed = true;
            }
        }
        string_builder_append_char(sb, '\n');
    }
    string_builder_append_char(sb, '\n');
}


void ir_build_str_program(StringBuilder *sb, Arena *scratch, Program program, RegisterPrintFn *print_register) {
    for (size_t i = 0; i < program.count; i++)
        ir_build_str_procedure(sb, scratch, &program.items[i], print_register);
    string_builder_append_char(sb, '\n');
}


void ir_print_program(Arena *scratch, Program program, RegisterPrintFn *print_register) {
    size_t mark = arena_mark(scratch);
    StringBuilder sb = { 0 };
    string_builder_create(&sb, scratch, 256);
    ir_build_str_program(&sb, scratch, program, print_register);
    fwrite(sb.string.data, 1, sb.string.len, stdout);
    arena_restore(scratch, mark);
}

void ir_print_procedure(Arena *scratch, const Procedure *procedure, RegisterPrintFn *print_register) {
    size_t mark = arena_mark(scratch);
    StringBuilder sb = { 0 };
    string_builder_create(&sb, scratch, 256);
    ir_build_str_procedure(&sb, scratch, procedure, print_register);
    fwrite(sb.string.data, 1, sb.string.len, stdout);
    arena_restore(scratch, mark);
}


bool operand_has_vreg(Operand operand, size_t vreg) {
    return (operand.type == OPERAND_VREG && operand.vreg == vreg) ||
            (operand.type == OPERAND_MEM &&
             ((operand.mem.base >= 0 && (size_t)operand.mem.base == vreg) ||
              (operand.mem.index >= 0 && (size_t)operand.mem.index == vreg)));
}


bool instr_defines_vreg(Instr instr, size_t vreg) {
    OpcodeDesc roles = opcode_description_table[instr.opcode];
    
    if (roles.dest == OPDR_DEF && operand_has_vreg(instr.dest, vreg)) return true;
    if (roles.arg1 == OPDR_DEF && operand_has_vreg(instr.arg1, vreg)) return true;
    if (roles.arg2 == OPDR_DEF && operand_has_vreg(instr.arg2, vreg)) return true;

    return false;
}


inline bool instr_match(Instr *instr, Opcode opcode, OperandType dest, OperandType arg1, OperandType arg2) {
    return (instr->opcode == opcode &&
                (dest == OPERAND_ANY || instr->dest.type == dest) &&
                (arg1 == OPERAND_ANY || instr->arg1.type == arg1) &&
                (arg2 == OPERAND_ANY || instr->arg2.type == arg2));
}


void operand_replace_vreg(Operand *operand, size_t find, size_t replace) {
    if (operand->type == OPERAND_VREG && operand->vreg == find) operand->vreg = replace;
    if (operand->type == OPERAND_MEM && operand->mem.index >= 0 && (size_t)operand->mem.index == find) operand->mem.index = (int)replace;
    if (operand->type == OPERAND_MEM && operand->mem.base >= 0 && (size_t)operand->mem.base == find) operand->mem.base = (int)replace;
}


void instr_replace_vreg(Instr *instr, size_t find, size_t replace) {
    operand_replace_vreg(&instr->dest, find, replace);
    operand_replace_vreg(&instr->arg1, find, replace);
    operand_replace_vreg(&instr->arg2, find, replace);
}


bool instr_uses_vreg(Instr instr, size_t vreg) {
    OpcodeDesc roles = opcode_description_table[instr.opcode];
    
    if (roles.dest == OPDR_USE && operand_has_vreg(instr.dest, vreg)) return true;
    if (roles.arg1 == OPDR_USE && operand_has_vreg(instr.arg1, vreg)) return true;
    if (roles.arg2 == OPDR_USE && operand_has_vreg(instr.arg2, vreg)) return true;

    return false;
}


int operand_collect_vregs(Operand opnd, size_t *out, int offset) {
    int size = 0;
    if (opnd.type == OPERAND_VREG) {
        out[offset + size++] = opnd.vreg;
    } else if (opnd.type == OPERAND_MEM) {
        if (opnd.mem.base != -1) out[offset + size++] = opnd.mem.base;
        if (opnd.mem.index != -1) out[offset + size++] = opnd.mem.index;
    }
    return size;
}


int instr_collect_used_vregs(Instr instr, size_t *out) {
    int size = 0;
    OpcodeDesc roles = opcode_description_table[instr.opcode];

    if (roles.dest == OPDR_USE) size += operand_collect_vregs(instr.dest, out, size);
    if (roles.arg1 == OPDR_USE) size += operand_collect_vregs(instr.arg1, out, size);
    if (roles.arg2 == OPDR_USE) size += operand_collect_vregs(instr.arg2, out, size);

    return size;
}


inline VregInfo vregi_from_sloti(SlotInfo slot) {
    return (VregInfo) {
        .size = slot.size,
        .sign = slot.sign
    };
}
