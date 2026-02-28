#include <stdio.h>

#include "common.h"
#include "ir.h"

int vreg_count = 0;
int stack_offset = 4;

IROp generate_load_imm_ir(InstList *ir, IROp imm) {
    Inst inst;
    IROp dest = {
        .type = IR_OP_VREG,
        .vreg = vreg_count++,
    };
    inst.type = INST_LOADI;
    inst.dest = dest;
    inst.arg1 = imm;

    array_append(*ir, inst);
    return dest;
}

IROp generate_load_ir(InstList *ir, int stack_offset) {
    Inst inst;
    inst.type = INST_LOAD;
    inst.dest.type = IR_OP_VREG;
    inst.dest.vreg = vreg_count++;
    inst.arg1.type = IR_OP_STACK;
    inst.arg1.stack_offset = stack_offset;
    array_append(*ir, inst);
    return inst.dest;
}

IROp generate_expr_ir(InstList *ir, AST *expr) {
    if (expr->type == AST_INT_LIT) {
        IROp imm;
        imm.type = IR_OP_IMM;
        imm.imm = expr->int_lit;
        IROp res = generate_load_imm_ir(ir, imm);
        return res;
    }
    if (expr->type == AST_SYMBOL) {
        SymbolData *var = get_symbol(expr->symbol.symbol_table, 
                                     expr->symbol.ident);
        return generate_load_ir(ir, var->stack_offset);
    }

    Inst inst;
    inst.dest.type = IR_OP_VREG;
    inst.dest.vreg = vreg_count++;
    inst.arg1 = generate_expr_ir(ir, expr->left);
    if (expr->right->type == AST_INT_LIT) {
        inst.arg2.type = IR_OP_IMM;
        inst.arg2.imm = expr->right->int_lit;
    } else {
        inst.arg2 = generate_expr_ir(ir, expr->right);
    }

    switch (expr->op) {
        case OP_PLUS:     inst.type = INST_ADD; break;
        case OP_MINUS:    inst.type = INST_SUB; break;
        case OP_MULTIPLY: inst.type = INST_MUL; break;
        case OP_DIVIDE:   inst.type = INST_DIV; break;
    }
    array_append(*ir, inst);

    return inst.dest;
}

void generate_store_ir(InstList *ir, int stack_offset, IROp vreg) {
    Inst store;
    store.type = INST_STORE;
    store.dest.type = IR_OP_STACK;
    store.dest.stack_offset = stack_offset;
    store.arg1 = vreg;
    array_append(*ir, store);
}

void generate_ir_from_ast(InstList *ir, AST *ast) {
    // Assumes AST is an AST_PROGRAM which should be guaranteed by parsing and static analysis
    // Could probably change function name to indicate that fact though
    for (int i = 0; i < ast->program.count; i++) {
        printf("out here\n");
        AST *statement = ast->program.items[i];
        switch (statement->type) {
            case AST_DECL: {
                IROp expr = generate_expr_ir(ir, statement->decl.expr);
                SymbolData *var = get_symbol(statement->decl.symbol->symbol.symbol_table, 
                                             statement->decl.symbol->symbol.ident);
                var->stack_offset = stack_offset;
                stack_offset += 4;

                generate_store_ir(ir, var->stack_offset, expr);
                break;
            }
            case AST_ASSIGNMENT: {
                IROp expr = generate_expr_ir(ir, statement->decl.expr);
                SymbolData *var = get_symbol(statement->decl.symbol->symbol.symbol_table, 
                                             statement->decl.symbol->symbol.ident);
                generate_store_ir(ir, var->stack_offset, expr);
                break;
            }
            case AST_RETURN:
                assert(0 && "TODO");
                break;
            case AST_BINARY_OP:
                generate_expr_ir(ir, ast->program.items[i]);
                break;
            default:
                assert(0 && "should never be in here");
        }
    }
}

void print_ir_op(IROp op) {
    if (op.type == IR_OP_IMM) {
        printf("%d", op.imm);
    } else {
        printf("r%d", op.vreg);
    }
}

void pretty_print_ir(InstList ir) {
    for (int i = 0; i < ir.count; i++) {
        Inst inst = ir.items[i];
        printf("%d: ", i);
        switch (inst.type) {
            case INST_ADD:
                printf("\tr%d = add ", inst.dest.vreg);
                print_ir_op(inst.arg1);
                printf(", ");
                print_ir_op(inst.arg2);
                printf("\n");
                break;
            case INST_SUB:
                printf("\tr%d = sub ", inst.dest.vreg);
                print_ir_op(inst.arg1);
                printf(", ");
                print_ir_op(inst.arg2);
                printf("\n");
                break;
            case INST_MUL:
                printf("\tr%d = mul ", inst.dest.vreg);
                print_ir_op(inst.arg1);
                printf(", ");
                print_ir_op(inst.arg2);
                printf("\n");
                break;
            case INST_DIV:
                printf("\tr%d = div ", inst.dest.vreg);
                print_ir_op(inst.arg1);
                printf(", ");
                print_ir_op(inst.arg2);
                printf("\n");
                break;
            case INST_LOAD:
                printf("\tr%d = [bp - %d]\n", inst.dest.vreg, inst.arg1.stack_offset);
                break;
            case INST_STORE:
                printf("\t[bp - %d] = r%d\n", inst.dest.stack_offset, inst.arg1.vreg); 
                break;
            case INST_MOV:
                printf("\tr%d = r%d\n", inst.dest.vreg, inst.arg1.vreg);
                break;
            case INST_LOADI:
                printf("\tr%d = loadi %d\n", inst.dest.vreg, inst.arg1.imm);
                break;
            default:
                printf("error unrecognized instruction in ir\n");
        }
    }
}

IntervalArray create_live_intervals_from_ir(InstList ir) {
    IntervalArray intervals;
    array_init(intervals, vreg_count);

    for (int vreg = 0; vreg < vreg_count; vreg++) {
        Interval interval = {0};
        interval.vreg = vreg;
        // find start
        for (int i = 0; i < ir.count; i++) {
            if (ir.items[i].dest.type == IR_OP_VREG && ir.items[i].dest.vreg == vreg) {
                interval.start = i;
                break;
            }
        }

        // find end
        for (int i = ir.count - 1; i >= 0; i--) {
            if (ir.items[i].arg1.type == IR_OP_VREG && ir.items[i].arg1.vreg == vreg) {
                interval.end = i;
                break;
            }
            if (ir.items[i].arg2.type == IR_OP_VREG && ir.items[i].arg2.vreg == vreg) {
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
        printf("r%d: [%d, %d]\n", intervals.items[i].vreg, intervals.items[i].start, intervals.items[i].end);
    }
}

const char *registers[] = {
    "eax", "ecx", "edx", "edi",
};

// TODO: will need to turn this into an interval set to perform register pre-allocation
// for procedures and operations that require specific registers
bool register_is_free[] = { 1, 1, 1, 1, };

// NOTE: assumes intervals is already sorted
LocationArray linear_scan_register_allocation(IntervalArray *intervals) {
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
