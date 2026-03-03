#include <stdio.h>

#include "ast.h"
#include "common.h"
#include "bytecode.h"

int vreg_count = 0;
int stack_offset = 0;

Operand generate_load_imm_bytecode(Bytecode *bytecode, Operand imm) {
    Instr instr;
    Operand dest = {
        .type = OPERAND_VREG,
        .vreg = vreg_count++,
    };
    instr.opcode = OPCODE_LOADI;
    instr.dest = dest;
    instr.arg1 = imm;

    array_append(*bytecode, instr);
    return dest;
}

Operand generate_load_bytecode(Bytecode *bytecode, int stack_offset) {
    Instr instr;
    instr.opcode = OPCODE_LOAD;
    instr.dest.type = OPERAND_VREG;
    instr.dest.vreg = vreg_count++;
    instr.arg1.type = OPERAND_STACK;
    instr.arg1.stack_offset = stack_offset;
    array_append(*bytecode, instr);
    return instr.dest;
}

Operand generate_expr_bytecode(Bytecode *bytecode, AST *expr) {
    if (expr->type == AST_INT_LIT) {
        Operand imm;
        imm.type = OPERAND_IMM;
        imm.imm = expr->int_lit;
        Operand res = generate_load_imm_bytecode(bytecode, imm);
        return res;
    }
    if (expr->type == AST_SYMBOL) {
        SymbolData *var = get_symbol(expr->symbol_table,
                                     expr->ident);
        return generate_load_bytecode(bytecode, var->stack_offset);
    }

    Instr instr;
    instr.dest.type = OPERAND_VREG;
    instr.dest.vreg = vreg_count++;
    instr.arg1 = generate_expr_bytecode(bytecode, expr->left);
    if (expr->right->type == AST_INT_LIT) {
        instr.arg2.type = OPERAND_IMM;
        instr.arg2.imm = expr->right->int_lit;
    } else {
        instr.arg2 = generate_expr_bytecode(bytecode, expr->right);
    }

    switch (expr->op) {
        case OP_PLUS:           instr.opcode = OPCODE_ADD; break;
        case OP_MINUS:          instr.opcode = OPCODE_SUB; break;
        case OP_MULTIPLY:       instr.opcode = OPCODE_MUL; break;
        case OP_DIVIDE:         instr.opcode = OPCODE_DIV; break;
        case OP_EQUALS:         instr.opcode = OPCODE_CMP_EQ; break;
        case OP_LESS_THAN:      instr.opcode = OPCODE_CMP_LT; break;
        case OP_GREATER_THAN:   instr.opcode = OPCODE_CMP_GT; break;
        case OP_LESS_EQUALS:    instr.opcode = OPCODE_CMP_LEQ; break;
        case OP_GREATER_EQUALS: instr.opcode = OPCODE_CMP_GEQ; break;
    }
    array_append(*bytecode, instr);

    return instr.dest;
}

void generate_store_bytecode(Bytecode *bytecode, int stack_offset, Operand vreg) {
    Instr store;
    store.opcode = OPCODE_STORE;
    store.dest.type = OPERAND_STACK;
    store.dest.stack_offset = stack_offset;
    store.arg1 = vreg;
    array_append(*bytecode, store);
}

void gen_bytecode_from_ast(Bytecode *bytecode, AST *ast) {
    // Assumes AST is an AST_PROGRAM which should be guaranteed by parsing and static analysis
    // Could probably change function name to indicate that fact though
    for (int i = 0; i < ast->count; i++) {
        AST *statement = ast->items[i];
        switch (statement->type) {
            case AST_DECL: {
                Operand expr = generate_expr_bytecode(bytecode, statement->expr);
                SymbolData *var = get_symbol(statement->symbol->symbol_table, 
                                             statement->symbol->ident);
                stack_offset += 4;
                var->stack_offset = stack_offset;

                generate_store_bytecode(bytecode, var->stack_offset, expr);
                break;
            }
            case AST_ASSIGNMENT: {
                Operand expr = generate_expr_bytecode(bytecode, statement->expr);
                SymbolData *var = get_symbol(statement->symbol->symbol_table, 
                                             statement->symbol->ident);
                generate_store_bytecode(bytecode, var->stack_offset, expr);
                break;
            }
            case AST_RETURN: {
                // TODO: in the future should probably have a return stack similar to function param stack
                // to support multiple return values
                Instr instr;
                instr.opcode = OPCODE_RET;
                instr.arg1 = generate_expr_bytecode(bytecode, statement->expr);
                array_append(*bytecode, instr);
                break;
            }
            case AST_BINARY_OP:
                generate_expr_bytecode(bytecode, ast->items[i]);
                break;
            default:
                assert(0 && "should never be in here");
        }
    }
}

void print_bytecode_op(Operand op) {
    if (op.type == OPERAND_IMM) {
        printf("%d", op.imm);
    } else if (op.type == OPERAND_VREG) {
        printf("r%d", op.vreg);
    } else {
        printf("%s", registers[op.reg]);
    }
}

void pretty_print_bytecode(Bytecode bytecode) {
    // TODO: this function needs to be reworked to support
    // multiple operand types. I need to decide on what types
    // of operands each instruction can support, and if immediate
    // arithemtic should have separate instructions or not (leaning towards no for now)
    for (int i = 0; i < bytecode.count; i++) {
        Instr instr = bytecode.items[i];
        printf("%d: ", i);
        switch (instr.opcode) {
            case OPCODE_ADD:
                printf("\t");
                print_bytecode_op(instr.dest);
                printf(" = add ");
                print_bytecode_op(instr.arg1);
                printf(", ");
                print_bytecode_op(instr.arg2);
                printf("\n");
                break;
            case OPCODE_SUB:
                printf("\t");
                print_bytecode_op(instr.dest);
                printf(" = sub ");
                print_bytecode_op(instr.arg1);
                printf(", ");
                print_bytecode_op(instr.arg2);
                printf("\n");
                break;
            case OPCODE_MUL:
                printf("\t");
                print_bytecode_op(instr.dest);
                printf(" = mul ");
                print_bytecode_op(instr.arg1);
                printf(", ");
                print_bytecode_op(instr.arg2);
                printf("\n");
                break;
            case OPCODE_DIV:
                printf("\t");
                print_bytecode_op(instr.dest);
                printf(" = div ");
                print_bytecode_op(instr.arg1);
                printf(", ");
                print_bytecode_op(instr.arg2);
                printf("\n");
                break;
            case OPCODE_LOAD:
                printf("\tr%d = [bp - %d]\n", instr.dest.vreg, instr.arg1.stack_offset);
                break;
            case OPCODE_STORE:
                printf("\t[bp - %d] = r%d\n", instr.dest.stack_offset, instr.arg1.vreg);
                break;
            case OPCODE_COPY:
                printf("\t");
                print_bytecode_op(instr.dest);
                printf(" = ");
                print_bytecode_op(instr.arg1);
                printf("\n");
                break;
            case OPCODE_LOADI:
                printf("\t");
                print_bytecode_op(instr.dest);
                printf(" = loadi ");
                print_bytecode_op(instr.arg1);
                printf("\n");
                break;
            case OPCODE_RET:
                printf("\tret r%d\n", instr.arg1.vreg);
                break;
            default:
                printf("error unrecognized instruction in bytecode\n");
        }
    }
}

IntervalArray create_live_intervals_from_bytecode(Bytecode bytecode) {
    IntervalArray intervals;
    array_init(intervals, vreg_count);

    for (int vreg = 0; vreg < vreg_count; vreg++) {
        Interval interval = {0};
        interval.vreg = vreg;
        // find start
        for (int i = 0; i < bytecode.count; i++) {
            if (bytecode.items[i].dest.type == OPERAND_VREG && bytecode.items[i].dest.vreg == vreg) {
                interval.start = i;
                break;
            }
        }

        // find end
        for (int i = bytecode.count - 1; i >= 0; i--) {
            if (bytecode.items[i].arg1.type == OPERAND_VREG && bytecode.items[i].arg1.vreg == vreg) {
                interval.end = i;
                break;
            }
            if (bytecode.items[i].arg2.type == OPERAND_VREG && bytecode.items[i].arg2.vreg == vreg) {
                interval.end = i;
                break;
            }
        }

        array_append(intervals, interval);
    }

    return intervals;
}

void print_live_intervals(IntervalArray intervals) {
    for (int i = 0; i < intervals.count; i++) {
        printf("r%d:\t[%d, %d)\n", intervals.items[i].vreg, intervals.items[i].start, intervals.items[i].end);
    }
}

const char *registers[] = {
    "eax", "ecx", "edx", "edi",
};

const char *registers_8bit_low[] = { // NOTE: this is meant to be a temporary solution for accessing lower 8 bits for setCC instructions
    "al", "cl", "dl", "dil",
};

// TODO: will need to turn this into an interval set to perform register pre-allocation
// for procedures and operations that require specific registers
bool register_is_free[] = { 1, 1, 1, 1, };

// NOTE: assumes intervals is already sorted
LocationArray linear_scan_register_allocation(IntervalArray *intervals, PhysRegs *pregs) {
    LocationArray locations = {0};
    array_init(locations, vreg_count);
    locations.count = vreg_count;
    IntervalArray active = {0};

    for (int i = 0; i < intervals->count; i++) {
        Interval *interval = &intervals->items[i];
        Location location = {0};
        location.type = LOC_REGISTER;
        int vreg = intervals->items[i].vreg;

        // Expire old intervals
        for (int j = 0; j < active.count; j++) {
            if (active.items[j].end > intervals->items[i].start) break;

            // free register
            register_is_free[locations.items[active.items[j].vreg].register_index] = true;

            // remove interval from active
            int k = j;
            while (k < active.count - 1) {
                active.items[k] = active.items[k + 1];
                k++;
            }
            active.count--;

        }

        // 4 is the amount of registers currently, will need to be changed for each platform
        if (active.count == 4) {
            // Spill interval i
            assert(0);
        } else {
            // Allocate register
            for (int k = 0; k < 4; k++) {
                if (register_is_free[k]) {
                    register_is_free[k] = false;
                    /*interval->loc.type = LOC_REGISTER;
                    interval->loc.register_index = k;*/
                    location.type = LOC_REGISTER;
                    location.register_index = k;
                    break;
                }
            }

            array_append(active, *interval);
            int insert_slot = active.count - 1;
            while (insert_slot > 0 && active.items[insert_slot - 1].end > interval->end) {
                active.items[insert_slot] = active.items[insert_slot - 1];
                insert_slot--;
            }
            active.items[insert_slot] = *interval;
        }

        locations.items[vreg] = location;
    }
    return locations;
}
