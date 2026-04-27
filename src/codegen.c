// include "ast.h"
// 
#include "codegen.h"
#include "common.h"
#include "symbol_table.h"
#include <stdio.h>

// void adhere_program(Program *program, PhysRegs *pregs) { // pregs arent used yet
//     for (int i = 0; i < program->count; i++) {
//         if (program->items[i].externed) continue;
//         program->items[i].bytecode = adhere_bytecode_to_machine_spec(program->items[i].bytecode, pregs);
//     }
// }

// void analyze_program(Program *program, PhysRegs *pregs) {
//     for (int i = 0; i < program->count; i++) {
//         Procedure *procedure = &program->items[i];
//         if (procedure->externed) continue;
//         procedure->intervals = create_live_intervals_from_bytecode(procedure->bytecode, procedure->vreg_count);

//         for (int i = 0; i < procedure->intervals.count - 1; i++) {
//             bool swapped = false;
//             for (int j = 0; j < procedure->intervals.count - i - 1; j++) {
//                 if (procedure->intervals.items[j].start > procedure->intervals.items[j+1].start) {
//                     Interval temp = procedure->intervals.items[j];
//                     procedure->intervals.items[j] = procedure->intervals.items[j+1];
//                     procedure->intervals.items[j+1] = temp;
//                     swapped = true;
//                 }
//             }
//             if (!swapped)
//                 break;
//         }

//         //print_live_intervals(procedure->intervals);

//         procedure->locations = linear_scan_register_allocation(&procedure->intervals, procedure->vreg_count, pregs);
//     }
// }


MachineCode emit_procedure_with(CodegenBackend *backend, Procedure procedure) {
    CodegenInterface iface = {
        .output = {0},
        .ctx = {
            .procedure_name = procedure.name,
        }
    };

    backend->emit_prologue(&iface, &procedure);

    for (int i = 0; i < procedure.flattened.count; i++, iface.ctx.current_instruction++) {
        Instr instr = procedure.flattened.items[i];
        EmitFn fn = backend->emit_machine_code_from[instr.opcode];
        assert(fn != NULL && "no emit handler for opcode");
        fn(&iface, instr);
    }

    backend->emit_epilogue(&iface, &procedure);

    return iface.output;
}


void compile_program_with(FILE *out, CodegenBackend *backend, Program *program) {

    backend->emit_symbols(out, program);
    backend->emit_entry(out, program);

    for (int i = 0; i < program->count; i++) {
        // if (program->items[i].externed) continue;
        fprintf(out, "%s:\n", program->items[i].name);
        MachineCode code = emit_procedure_with(backend, program->items[i]);
        backend->print_machine_code(out, &code);
    }
}


// Bytecode adhere_bytecode_to_machine_spec(Bytecode bytecode, PhysRegs *pregs) {
//     Bytecode machine_code = {0};
//     for (int i = 0; i < bytecode.count; i++) {
//         Instr instr = bytecode.items[i];
//         switch (instr.opcode) {
//             // NOTE: I dont think its required for the equality operations to be changed as
//             // CMP sets flags and a second instruction (i.e setl) is required to get result
//             case OPCODE_ADD:
//             case OPCODE_SUB:
//             case OPCODE_MUL: {
//                 Instr copy;
//                 copy.opcode = OPCODE_COPY;
//                 copy.dest = instr.dest;
//                 copy.arg1 = instr.arg1;
//                 array_append(machine_code, copy);
//                 instr.arg1 = instr.dest;
//                 array_append(machine_code, instr);
//                 break;
//             }
//             case OPCODE_DIV: {
//                 assert(0);
//                 break;
//             }
//             case OPCODE_RET: {
//                 // TODO: add abi requirement here
//                 array_append(machine_code, instr);
//                 break;
//             }
//             case OPCODE_LOAD:
//             case OPCODE_LOAD_INDEX:
//             case OPCODE_LOAD_ADDR:
//             case OPCODE_STORE:
//             case OPCODE_COPY:
//             case OPCODE_LOADI:
//             case OPCODE_CMP_EQ:
//             case OPCODE_CMP_LT:
//             case OPCODE_CMP_GT:
//             case OPCODE_CMP_LEQ:
//             case OPCODE_CMP_GEQ:
//             case OPCODE_JMP:
//             case OPCODE_JMP_NEQ:
//             case OPCODE_JMP_EQ:
//             case OPCODE_JMP_LT:
//             case OPCODE_JMP_GT:
//             case OPCODE_JMP_LE:
//             case OPCODE_JMP_GE:
//             case OPCODE_CALL:
//             case OPCODE_LABEL:
//             case OPCODE_PARAM:
//                 array_append(machine_code, instr);
//                 break;
//             default:
//                 assert(0);
//         }
//     }
//     free(bytecode.items);
//     return machine_code;
// }
