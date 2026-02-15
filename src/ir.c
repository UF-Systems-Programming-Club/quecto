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
    // TODO: will potentially have to add sorting functionality for the interval list.
    // for now this technically doesn't work because of how we treat arguments in an
    // instruction but I need to do some other things so I am pushing for now
    IntervalList intervals = {0};

    for (int i = 0; i < current_register; i++) {
        LiveInterval interval;

        // get start of interval
        for (int j = 0; j < ir.count; j++) {
            Inst inst = ir.items[j];
            if (inst.dest == i || inst.arg1 == i || inst.arg2 == i) {
                interval.start = j;
                break;
            }
        }

        // get end of interval
        for (int j = ir.count - 1; j >= 0; j--) {
            Inst inst = ir.items[j];
            if (inst.dest == i || inst.arg1 == i || inst.arg2 == i) {
                interval.end = j;
                break;
            }
        }

        array_append(intervals, interval);
    }

    return intervals;
}

void print_live_intervals(IntervalList intervals) {
    for (int i = 0; i < intervals.count; i++) {
        printf("r%d: [%d, %d]\n", i, intervals.items[i].start, intervals.items[i].end);
    }
}
