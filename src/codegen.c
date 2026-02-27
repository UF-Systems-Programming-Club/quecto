#include "ir.h"
#include "codegen.h"

// NOTE: for linux x64 only rn. makes it so that it is 2AC instead of 3AC, and will do more in the future
InstList adhere_ir_to_machine_spec(InstList ir) {
    InstList machine_ir = {0};
    for (int i = 0; i < ir.count; i++) {
        Inst inst = ir.items[i];
        switch (inst.type) {
            case INST_ADD:
            case INST_SUB:
            case INST_MUL:
            case INST_DIV: // TODO: div can't be here for x64
            {
                Inst mov;
                mov.type = INST_MOV;
                mov.dest = inst.dest;
                mov.arg1 = inst.arg1;
                array_append(machine_ir, mov);
                inst.arg1 = inst.dest;
                array_append(machine_ir, inst);
                break;
            }
            case INST_LOADI:
                array_append(machine_ir, inst);
                break;
            default:
                assert(0);
        }
    }
    return machine_ir;
}

void generate_assembly_from_ir(FILE *out, InstList ir, LocationArray location) {
    // need to implement register rewriter
    for (int i = 0; i < ir.count; i++) {
        Inst inst = ir.items[i];
        int dest = location.items[inst.dest.vreg].register_index;
        int arg1 = location.items[inst.arg1.vreg].register_index;
        int arg2 = location.items[inst.arg2.vreg].register_index;
        switch (inst.type) {
            case INST_ADD:
                switch (inst.arg2.type) {
                    case IR_OP_VREG:
                        fprintf(out,
                                "\tadd\t%s, %s\n",
                                registers[dest],
                                registers[arg2]);
                        break;
                    case IR_OP_IMM:
                        fprintf(out,
                                "\tadd\t%s, %d\n",
                                registers[dest],
                                inst.arg2.imm);
                        break;
                }
                break;
            case INST_SUB:
                switch (inst.arg2.type) {
                    case IR_OP_VREG:
                        fprintf(out,
                                "\tsub\t%s, %s\n",
                                registers[dest],
                                registers[arg2]);
                        break;
                    case IR_OP_IMM:
                        fprintf(out,
                                "\tsub\t%s, %d\n",
                                registers[dest],
                                inst.arg2.imm);
                        break;
                }
                break;
            case INST_MUL:
                switch (inst.arg2.type) {
                    case IR_OP_VREG:
                        fprintf(out,
                                "\timul\t%s, %s\n",
                                registers[dest],
                                registers[arg2]);
                        break;
                    case IR_OP_IMM:
                        fprintf(out,
                                "\timul\t%s, %d\n",
                                registers[dest],
                                inst.arg2.imm);
                        break;
                }
                break;
            case INST_DIV:
                assert(0);
                break;
            case INST_MOV:
                if (dest != arg1) {
                    fprintf(out, "\tmov\t%s, %s\n",
                            registers[dest],
                            registers[arg1]);
                }
                break;
            case INST_LOADI:
                fprintf(out, "\tmov\t%s, %d\n", registers[dest], inst.arg1.imm);
                break;
            default:
                assert(0);
        }
    }
}
