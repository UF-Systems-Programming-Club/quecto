// include "ast.h"
// 
#include "codegen.h"
#include "common.h"
#include "ir.h"
#include "symbol_table.h"

#include <stdio.h>

#define MLBL(l) ((MachineOperand){.type = MOPERAND_LABEL, .label = (l) })


MachineCode emit_procedure_with(CodegenBackend *backend, Procedure procedure, SymbolTable *globals) {
    CodegenInterface iface = {
        .output = {0},
        .vregs = &procedure.vregs,
        .slots = &procedure.slots,
        .globals = globals,
        .arg_count = 0,
    };

    backend->calculate_offsets(&iface);
    backend->emit_prologue(&iface, &procedure);

    for (int i = 0; i < procedure.cfg.count; i++) {
        int b = procedure.cfg.rpo_list[i];
        BasicBlock *block = &procedure.cfg.items[b];
        
        char *buf = malloc(16);
        snprintf(buf, 16, "\t.L_BLK%d", b);
        MachineInstr instr = (MachineInstr) { .instruction = 50, .dest = MLBL(buf)};
        array_append(iface.output, instr);

        for (int j = 0; j < block->bytecode.count; j++) {
            Instr instr = block->bytecode.items[j];
            EmitFn fn = backend->emit_machine_code_from[instr.opcode];
            assert(fn != NULL && "no emit handler for opcode");
            fn(&iface, instr);
        }

        Instr branch = block->terminator;

        switch (branch.opcode) {
            case OPCODE_JMP:
                if (procedure.cfg.idom[branch.successor[0]] == b)
                    continue;
                branch.dest = (Operand) { .type = OPERAND_BLOCK, .block = branch.successor[0] };
                break;
            case OPCODE_RET:
                break;
            default:
                branch.dest = (Operand) { .type = OPERAND_BLOCK, .block = branch.successor[1]};
                branch.opcode = opposite_opcode_table[branch.opcode];
                break;
        }
        
        EmitFn fn = backend->emit_machine_code_from[branch.opcode];
        assert(fn != NULL && "no emit handler for opcode");
        fn(&iface, branch);
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
        MachineCode code = emit_procedure_with(backend, program->items[i], program->symbols);
        backend->print_machine_code(out, &code);
    }
}
