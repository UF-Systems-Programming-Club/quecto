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
            case INST_MUL: {
                Inst mov;
                mov.type = INST_MOV;
                mov.dest = inst.dest;
                mov.arg1 = inst.arg1;
                array_append(machine_ir, mov);
                inst.arg1 = inst.dest;
                array_append(machine_ir, inst);
                break;
            }
            case INST_DIV: {

                break;
            }
            case INST_LOAD:
            case INST_STORE:
            case INST_MOV:
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
        switch (inst.type) {
            case INST_ADD:
                switch (inst.arg2.type) {
                    case IR_OP_VREG:
                        fprintf(out,
                                "\tadd\t%s, %s\n",
                                registers[location.items[inst.dest.vreg].register_index],
                                registers[location.items[inst.arg2.vreg].register_index]);
                        break;
                    case IR_OP_IMM:
                        fprintf(out,
                                "\tadd\t%s, %d\n",
                                registers[location.items[inst.dest.vreg].register_index],
                                inst.arg2.imm);
                        break;
                }
                break;
            case INST_SUB:
                switch (inst.arg2.type) {
                    case IR_OP_VREG:
                        fprintf(out,
                                "\tsub\t%s, %s\n",
                                registers[location.items[inst.dest.vreg].register_index],
                                registers[location.items[inst.arg2.vreg].register_index]);
                        break;
                    case IR_OP_IMM:
                        fprintf(out,
                                "\tsub\t%s, %d\n",
                                registers[location.items[inst.dest.vreg].register_index],
                                inst.arg2.imm);
                        break;
                }
                break;
            case INST_MUL:
                switch (inst.arg2.type) {
                    case IR_OP_VREG:
                        fprintf(out,
                                "\timul\t%s, %s\n",
                                registers[location.items[inst.dest.vreg].register_index],
                                registers[location.items[inst.arg2.vreg].register_index]);
                        break;
                    case IR_OP_IMM:
                        fprintf(out,
                                "\timul\t%s, %d\n",
                                registers[location.items[inst.dest.vreg].register_index],
                                inst.arg2.imm);
                        break;
                }
                break;
            case INST_STORE:
                fprintf(out, "\tmov\t[rbp - %d], %s\n", inst.dest.stack_offset, registers[location.items[inst.arg1.vreg].register_index]);
                break;
            case INST_LOAD:
                fprintf(out, "\tmov\t%s, [rbp - %d]\n", registers[location.items[inst.dest.vreg].register_index], inst.arg1.stack_offset);
                break;
            case INST_DIV:
                assert(0);
                break;
            case INST_MOV:
                if (location.items[inst.dest.vreg].register_index != location.items[inst.arg1.vreg].register_index) {
                    fprintf(out, "\tmov\t%s, %s\n",
                            registers[location.items[inst.dest.vreg].register_index],
                            registers[location.items[inst.arg1.vreg].register_index]);
                }
                break;
            case INST_LOADI:
                fprintf(out, "\tmov\t%s, %d\n", registers[location.items[inst.dest.vreg].register_index], inst.arg1.imm);
                break;
            default:
                assert(0);
        }
    }
}
