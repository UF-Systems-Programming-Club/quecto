#include "ast.h"
#include "bytecode.h"
#include "codegen.h"
#include "common.h"
#include "symbol_table.h"
#include <stdio.h>

const char *cmp_x86_instruction_from_opcode[] = {
    [OPCODE_CMP_EQ] = "sete",
    [OPCODE_CMP_LT] = "setl",
    [OPCODE_CMP_GT] = "setg",
    [OPCODE_CMP_LEQ] = "setle",
    [OPCODE_CMP_GEQ] = "setge"
};

void adhere_program(Program *program, PhysRegs *pregs) { // pregs arent used yet
    for (int i = 0; i < program->count; i++) {
        program->items[i].bytecode = adhere_bytecode_to_machine_spec(program->items[i].bytecode, pregs);
    }
}

void analyze_program(Program *program, PhysRegs *pregs) {
    for (int i = 0; i < program->count; i++) {
        Procedure *procedure = &program->items[i];
        procedure->intervals = create_live_intervals_from_bytecode(procedure->bytecode, procedure->vreg_count);

        for (int i = 0; i < procedure->intervals.count - 1; i++) {
            bool swapped = false;
            for (int j = 0; j < procedure->intervals.count - i - 1; j++) {
                if (procedure->intervals.items[j].start > procedure->intervals.items[j+1].start) {
                    Interval temp = procedure->intervals.items[j];
                    procedure->intervals.items[j] = procedure->intervals.items[j+1];
                    procedure->intervals.items[j+1] = temp;
                    swapped = true;
                }
            }
            if (!swapped)
                break;
        }

        //print_live_intervals(procedure->intervals);

        procedure->locations = linear_scan_register_allocation(&procedure->intervals, procedure->vreg_count, pregs);
    }
}

void compile_program(FILE *out, Program *program) {
    for (int i = 0; i < program->count; i++) {
        emit_procedure_assembly(out, program->items[i]);
    }
}

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
            case OPCODE_JMP:
            case OPCODE_JMP_NEQ:
            case OPCODE_CALL:
            case OPCODE_LABEL:
            case OPCODE_PARAM:
                array_append(machine_code, instr);
                break;
            default:
                assert(0);
        }
    }
    free(bytecode.items);
    return machine_code;
}

const char *arg_registers[] = { "edi", "esi", "edx", "ecx" };

void emit_procedure_assembly(FILE *out, Procedure procedure) {
    Operand args[MAX_PARAMS]; // for param opcode
    size_t args_count = 0;

    Bytecode bytecode = procedure.bytecode;
    LocationArray location = procedure.locations;
    IntervalArray interval = procedure.intervals;

    fprintf(out, "%s:\n", procedure.name);
    fprintf(out, "\tpush\trbp\n");
    fprintf(out, "\tmov\trbp, rsp\n");

    if (procedure.local_var_size > 0) fprintf(out, "\tsub\trsp, %d\n", (procedure.local_var_size / 16 + 1) * 16 );
    int offset = 0;
    for (int j = 0; j < procedure.param_count; j++) {
        offset += 4;
        fprintf(out, "\tmov\t[rbp - %d], %s\n", offset, arg_registers[j]);
    }

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
                // fprintf(out, "\tret\n");
                fprintf(out, "\tjmp .%s.end\n", procedure.name);
                break;
            case OPCODE_JMP_NEQ:
                fprintf(out,
                        "\tcmp\t%s,%d\n",
                        registers[location.items[instr.arg1.vreg].register_index],
                        1);
                fprintf(out, "\tjne\t%s\n", instr.dest.label_name);
                break;
            case OPCODE_JMP:
                fprintf(out, "\tjmp\t%s\n", instr.dest.label_name);
                break;
            case OPCODE_PARAM: {
                args[args_count++] = instr.arg1;
                break;
            }
            case OPCODE_CALL: {

                // preliminary spilling live registers to slots after local variables

                int spill_offset = 0;
                for (int j = 0; j < interval.count; j++) {
                    Interval span = interval.items[j];
                    if (span.start < i && i < span.end) {
                        spill_offset += 4;
                        fprintf(out, "\tmov\t[rbp - %d], %s\n", spill_offset + offset, registers[location.items[span.vreg].register_index]);
                    }
                }

                for (int j = 0; j < args_count; j++) {
                    fprintf(out, "\tmov\t%s, %s\n", arg_registers[j], registers[location.items[args[j].vreg].register_index]);
                }
                args_count = 0;

                fprintf(out, "\tcall\t%s\n", instr.arg1.label_name);
                if (instr.dest.type == OPERAND_VREG)
                    fprintf(out, "\tmov\t%s, %s\n",
                                    registers[location.items[instr.dest.vreg].register_index],
                                    "eax");
                spill_offset = 0;
                for (int j = 0; j < interval.count; j++) {
                    Interval span = interval.items[j];
                    if (span.start < i && i < span.end) {
                        spill_offset += 4;
                        fprintf(out, "\tmov\t%s, [rbp - %d]\n", registers[location.items[span.vreg].register_index], offset + spill_offset);
                    }
                }
                break;
            }
            case OPCODE_LABEL:
                fprintf(out, "%s:\n", instr.dest.label_name);
                break;
            default:
                assert(0);
        }
    }

    fprintf(out, ".%s.end:\n", procedure.name);
    if (procedure.local_var_size > 0) fprintf(out, "\tadd\trsp, %d\n", (procedure.local_var_size / 16 + 1) * 16 );
    fprintf(out, "\tpop\trbp\n");
    fprintf(out, "\tret\n");
}
