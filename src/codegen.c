#include "bytecode.h"
#include "codegen.h"

const char *cmp_x86_instruction_from_opcode[] = {
    [OPCODE_CMP_EQ] = "sete",
    [OPCODE_CMP_LT] = "setl",
    [OPCODE_CMP_GT] = "setg",
    [OPCODE_CMP_LEQ] = "setle",
    [OPCODE_CMP_GEQ] = "setge"
};

// NOTE: for linux x64 only rn. makes it so that it is 2AC instead of 3AC, and will do more in the future
Bytecode adhere_bytecode_to_machine_spec(Bytecode bytecode, PhysRegs *pregs) {
    Bytecode machine_bytecode = {0};
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
                array_append(machine_bytecode, copy);
                instr.arg1 = instr.dest;
                array_append(machine_bytecode, instr);
                break;
            }
            case OPCODE_DIV: {
                // TODO: should have enums for registers for each backend
                Instr dividend;
                dividend.opcode = OPCODE_LOADI;
                dividend.dest.type = OPERAND_REG;
                dividend.dest.reg = 2; // TODO
                dividend.arg1.type = OPERAND_IMM;
                dividend.arg1.imm = 0;
                array_append(machine_bytecode, dividend);
                dividend.opcode = OPCODE_COPY;
                dividend.dest.reg = 0; //TODO
                dividend.arg1 = instr.arg1;
                array_append(machine_bytecode, dividend);

                Instr copy;
                copy.dest = instr.dest;
                copy.arg1.type = OPERAND_REG;
                copy.arg1.reg = 0;
                
                instr.dest.type = OPERAND_REG;
                instr.dest.reg = 0;
                instr.arg1.type = OPERAND_REG;
                instr.arg1.reg = 2;
                instr.

                array_append(machine_bytecode, instr);
                array_append(machine_bytecode, copy);
                break;
            }
            case OPCODE_RET: {
                // TODO: add abi requirement here
                array_append(machine_bytecode, instr);
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
                array_append(machine_bytecode, instr);
                break;
            default:
                assert(0);
        }
    }
    free(bytecode.items);
    return machine_bytecode;
}

void generate_assembly_from_bytecode(FILE *out, Bytecode bytecode, LocationArray location) {
    // need to implement register rewriter
    for (int i = 0; i < bytecode.count; i++) {
        Instr instr = bytecode.items[i];
        switch (instr.opcode) {
            case OPCODE_CMP_EQ:
            case OPCODE_CMP_LT:
            case OPCODE_CMP_GT:
            case OPCODE_CMP_LEQ:
            case OPCODE_CMP_GEQ:
                switch (instr.arg2.type) {
                    case OPERAND_VREG:
                        fprintf(out,
                            "\tcmp\t%s,%s\n",
                            registers[location.items[instr.arg1.vreg].register_index],
                            registers[location.items[instr.arg2.vreg].register_index]);
                        break;
                    case OPERAND_IMM:
                        fprintf(out,
                                "\tcmp\t%s, %d\n",
                                registers[location.items[instr.arg1.vreg].register_index],
                                instr.arg2.imm);
                        break;
                }
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
                switch (instr.arg2.type) {
                    case OPERAND_VREG:
                        fprintf(out,
                                "\tadd\t%s, %s\n",
                                registers[location.items[instr.dest.vreg].register_index],
                                registers[location.items[instr.arg2.vreg].register_index]);
                        break;
                    case OPERAND_IMM:
                        fprintf(out,
                                "\tadd\t%s, %d\n",
                                registers[location.items[instr.dest.vreg].register_index],
                                instr.arg2.imm);
                        break;
                }
                break;
            case OPCODE_SUB:
                switch (instr.arg2.type) {
                    case OPERAND_VREG:
                        fprintf(out,
                                "\tsub\t%s, %s\n",
                                registers[location.items[instr.dest.vreg].register_index],
                                registers[location.items[instr.arg2.vreg].register_index]);
                        break;
                    case OPERAND_IMM:
                        fprintf(out,
                                "\tsub\t%s, %d\n",
                                registers[location.items[instr.dest.vreg].register_index],
                                instr.arg2.imm);
                        break;
                }
                break;
            case OPCODE_MUL:
                switch (instr.arg2.type) {
                    case OPERAND_VREG:
                        fprintf(out,
                                "\timul\t%s, %s\n",
                                registers[location.items[instr.dest.vreg].register_index],
                                registers[location.items[instr.arg2.vreg].register_index]);
                        break;
                    case OPERAND_IMM:
                        fprintf(out,
                                "\timul\t%s, %d\n",
                                registers[location.items[instr.dest.vreg].register_index],
                                instr.arg2.imm);
                        break;
                }
                break;
            case OPCODE_STORE:
                fprintf(out, "\tmov\t[rbp - %d], %s\n", instr.dest.stack_offset, registers[location.items[instr.arg1.vreg].register_index]);
                break;
            case OPCODE_LOAD:
                fprintf(out, "\tmov\t%s, [rbp - %d]\n", registers[location.items[instr.dest.vreg].register_index], instr.arg1.stack_offset);
                break;
            case OPCODE_DIV:
                assert(0);
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
