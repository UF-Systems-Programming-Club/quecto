#include <stdio.h>

#include "common.h"
#include "ir.h"

int current_register = 0;

int generate_expr_ir(InstList *ir, AST *expr) {
    if (expr->type == AST_INT_LIT) {
        Inst inst = {
            .type = INST_LOAD,
            .dest = current_register,
            .arg1 = expr->int_lit
        };
        // TODO: do i want to create the ir like this?
        array_append((*ir), inst);
        return current_register++;
    }

    int arg1 = generate_expr_ir(ir, expr->left);
    int arg2 = generate_expr_ir(ir, expr->right);

    Inst inst;
    inst.dest = current_register;
    inst.arg1 = arg1;
    inst.arg2 = arg2;
    switch (expr->op) {
        case OP_PLUS:
            inst.type = INST_ADD;
            break;
        case OP_MINUS:
            inst.type = INST_SUB;
            break;
        case OP_MULTIPLY:
            inst.type = INST_MUL;
            break;
        case OP_DIVIDE:
            inst.type = INST_DIV;
            break;
    }
    array_append((*ir), inst);
    return current_register++;
}

void generate_ir_from_ast(InstList *ir, AST *ast) {
    // TODO: assumes ast is an expression only for now
    generate_expr_ir(ir, ast);
}

void pretty_print_ir(InstList ir) {
    for (int i = 0; i < ir.count; i++) {
        Inst inst = ir.items[i];
        printf("%d: ", i);
        switch (inst.type) {
            case INST_ADD:
                printf("r%d = add r%d, r%d\n", inst.dest, inst.arg1, inst.arg2);
                break;
            case INST_SUB:
                printf("r%d = sub r%d, r%d\n", inst.dest, inst.arg1, inst.arg2);
                break;
            case INST_MUL:
                printf("r%d = mul r%d, r%d\n", inst.dest, inst.arg1, inst.arg2);
                break;
            case INST_DIV:
                printf("r%d = div r%d, r%d\n", inst.dest, inst.arg1, inst.arg2);
                break;
            case INST_LOAD:
                printf("r%d = %d\n", inst.dest, inst.arg1);
                break;
            default:
                printf("error unrecognized instruction in ir\n");
        }
    }
}

IntervalList create_live_intervals_from_ir(InstList ir) {
    // NOTE: will potentially have to add sorting functionality for the interval list.
    // assumes instructions, registers, and intervals are in the same order in their
    // respective arrays
    IntervalList intervals = {0};

    for (int i = 0; i < current_register; i++) {
        Interval interval;

        // get start of interval
        for (int j = 0; j < ir.count; j++) {
            Inst inst = ir.items[j];
            if (inst.dest == i) {
                interval.start = j;
                break;
            }
        }

        // get end of interval
        for (int j = ir.count - 1; j >= 0; j--) {
            Inst inst = ir.items[j];
            bool exit = false;
            switch (inst.type) {
                case INST_ADD:
                    if (inst.dest == i || inst.arg1 == i || inst.arg2 == i) {
                        interval.end = j;
                        exit = true;
                    }
                    break;
                case INST_SUB:
                    if (inst.dest == i || inst.arg1 == i || inst.arg2 == i) {
                        interval.end = j;
                        exit = true;
                    }
                    break;
                case INST_MUL:
                    if (inst.dest == i || inst.arg1 == i || inst.arg2 == i) {
                        interval.end = j;
                        exit = true;
                    }
                    break;
                case INST_DIV:
                    if (inst.dest == i || inst.arg1 == i || inst.arg2 == i) {
                        interval.end = j;
                        exit = true;
                    }
                    break;
                case INST_LOAD:
                    if (inst.dest == i) {
                        interval.end = j;
                        exit = true;
                    }
                    break;
            }
            if (exit) break;
        }

        array_append(intervals, interval);
    }

    return intervals;
}

const char *registers[] = {
    "eax", "ecx", "edx", "edi",
};

// TODO: will need to turn this into an interval set to perform register pre-allocation
// for procedures and operations that require specific registers
bool register_is_free[] = { 1, 1, 1, 1, };

void print_live_intervals(IntervalList intervals) {
    for (int i = 0; i < intervals.count; i++) {
        printf("r%d: [%d, %d] @ ", i, intervals.items[i].start, intervals.items[i].end);
        switch (intervals.items[i].loc.type) {
            case LOC_STACK:
                printf("[rbp - %d]\n", intervals.items[i].loc.stack_offset);
            case LOC_REGISTER:
                printf("%s\n", registers[intervals.items[i].loc.register_index]);
        }
    }
}

void linear_scan_register_allocation(IntervalList *intervals) {
    // NOTE: assumes intervals is sorted in order of increasing start point already. might change where that is
    // not true
    IntervalList active = {0};
    for (int i = 0; i < intervals->count; i++) {
        Interval *interval = &intervals->items[i];

        // Expire old intervals
        for (int j = 0; j < active.count; j++) {
            if (active.items[j].end >= intervals->items[i].start) break;

            // free register
            register_is_free[active.items[j].loc.register_index] = true;

            // remove interval from active
            int k = j;
            while (k < active.count - 1) {
                active.items[k] = active.items[k + 1];
                k++;
            }
            active.count--;

        }

        // 4 is the amount of registers, will need to be changed for each platform
        if (active.count == 4) {
            // Spill interval i
            // assert(0);
            printf("in here\n");
        } else {
            // allocate register
            for (int k = 0; k < 4; k++) {
                if (register_is_free[k]) {
                    register_is_free[k] = false;
                    interval->loc.type = LOC_REGISTER;
                    interval->loc.register_index = k;
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
    }
}
