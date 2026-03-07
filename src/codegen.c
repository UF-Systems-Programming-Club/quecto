#include "bytecode.h"
#include "codegen.h"

const char *cmp_x86_instruction_from_opcode[] = {
    [OPCODE_CMP_EQ] = "sete",
    [OPCODE_CMP_LT] = "setl",
    [OPCODE_CMP_GT] = "setg",
    [OPCODE_CMP_LEQ] = "setle",
    [OPCODE_CMP_GEQ] = "setge"
};

Bytecode adhere_bytecode_to_machine_spec(Bytecode bytecode, PhysRegs *pregs) {
    Bytecode machine_code = {0};
    for (int i = 0; i < bytecode.count; i++) {
        Instr instr = bytecode.items[i];
        switch (instr.opcode) {
            // NOTE: I dont think its required for the equality operations to be changed as
            // CMP sets flags and a second instruction (i.e setl) is required to get result
            case OPCODE_ADD:
            case OPCODE_SUB:
            case OPCODE_MUL: {
                Instr copy;
                copy.opcode = OPCODE_COPY;
                copy.dest = instr.dest;
                copy.arg1 = instr.arg1;
                array_append(machine_code, copy);
                instr.arg1 = instr.dest;
                array_append(machine_code, instr);
                break;
            }
            case OPCODE_DIV: {
                assert(0);
                break;
            }
            case OPCODE_RET: {
                // TODO: add abi requirement here
                array_append(machine_code, instr);
                break;
            }
            case OPCODE_LOAD:
            case OPCODE_STORE:
            case OPCODE_COPY:
            case OPCODE_LOADI:
            case OPCODE_CMP_EQ:
            case OPCODE_CMP_LT:
            case OPCODE_CMP_GT:
            case OPCODE_CMP_LEQ:
            case OPCODE_CMP_GEQ:
                array_append(machine_code, instr);
                break;
            default:
                assert(0);
        }
    }
    free(bytecode.items);
    return machine_code;
}

void emit_assembly_from_bytecode(FILE *out, Bytecode bytecode, LocationArray location) {
    for (int i = 0; i < bytecode.count; i++) {
        Instr instr = bytecode.items[i];
        switch (instr.opcode) {
            case OPCODE_CMP_EQ:
            case OPCODE_CMP_LT:
            case OPCODE_CMP_GT:
            case OPCODE_CMP_LEQ:
            case OPCODE_CMP_GEQ:
                fprintf(out,
                        "\tcmp\t%s,%s\n",
                        registers[location.items[instr.arg1.vreg].register_index],
                        registers[location.items[instr.arg2.vreg].register_index]);
                fprintf(out,
                        "\t%s\t%s\n",
                        cmp_x86_instruction_from_opcode[instr.opcode],
                        registers_8bit_low[location.items[instr.dest.vreg].register_index]); // set 8-bit reg
                fprintf(out,
                        "\tmovzx\t%s, %s\n",
                        registers[location.items[instr.dest.vreg].register_index],
                        registers_8bit_low[location.items[instr.dest.vreg].register_index]); // zero extend to rest (not optimal but we are only dealing with 32-bit regs in x86 so far)
                break;
            case OPCODE_ADD:
                fprintf(out,
                        "\tadd\t%s, %s\n",
                        registers[location.items[instr.dest.vreg].register_index],
                        registers[location.items[instr.arg2.vreg].register_index]);
                break;
            case OPCODE_SUB:
                fprintf(out,
                        "\tsub\t%s, %s\n",
                        registers[location.items[instr.dest.vreg].register_index],
                        registers[location.items[instr.arg2.vreg].register_index]);
                break;
            case OPCODE_MUL:
                fprintf(out,
                        "\timul\t%s, %s\n",
                        registers[location.items[instr.dest.vreg].register_index],
                        registers[location.items[instr.arg2.vreg].register_index]);
                break;
            case OPCODE_DIV:
                fprintf(out,
                        "\tdiv\t%s\n",
                        registers[location.items[instr.arg1.vreg].register_index]);
                break;
            case OPCODE_STORE:
                fprintf(out, "\tmov\t[rbp - %d], %s\n", instr.dest.stack_offset, registers[location.items[instr.arg1.vreg].register_index]);
                break;
            case OPCODE_LOAD:
                fprintf(out, "\tmov\t%s, [rbp - %d]\n", registers[location.items[instr.dest.vreg].register_index], instr.arg1.stack_offset);
                break;
            case OPCODE_COPY:
                if (location.items[instr.dest.vreg].register_index != location.items[instr.arg1.vreg].register_index) {
                    fprintf(out, "\tmov\t%s, %s\n",
                            registers[location.items[instr.dest.vreg].register_index],
                            registers[location.items[instr.arg1.vreg].register_index]);
                }
                break;
            case OPCODE_LOADI:
                fprintf(out, "\tmov\t%s, %d\n", registers[location.items[instr.dest.vreg].register_index], instr.arg1.imm);
                break;
            case OPCODE_RET:
                fprintf(out, "\tmov\t%s, %s\n", registers[0], registers[location.items[instr.arg1.vreg].register_index]);
                fprintf(out, "\tret\n");
                break;
            default:
                assert(0);
        }
    }
}
