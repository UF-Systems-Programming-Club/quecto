#include <stdio.h>

#include "ast.h"
#include "common.h"
#include "bytecode.h"
#include "symbol_table.h"

int vreg_count = 0;
int stack_offset = 0;

// NOTE: in the future these instructions will hold more information like
// size of type, signed or unsigned, etc;

Operand gen_add_instr(Bytecode *bytecode, Operand left, Operand right) {
}

Operand gen_jmp(Bytecode *bytecode, Operand label) {
    Instr jmp;
    jmp.opcode = OPCODE_JMP;
    jmp.dest = label;
    array_append(*bytecode, jmp);
    return label;
}

Operand gen_conditional_jump(Bytecode *bytecode, Opcode type, Operand label, Operand condition) {
    Instr jmp;
    jmp.opcode = type;
    jmp.dest = label;
    jmp.arg1 = condition;
    array_append(*bytecode, jmp);
    return label;
}

static int count = 0;

Operand create_label() {
    Operand lab;
    lab.type = OPERAND_LABEL;
    lab.label_name = calloc(8, sizeof(char));
    snprintf(lab.label_name, 8, ".LBL%d", count++);
    return lab;
}

Operand create_label_from(const char *str) {
    Operand lab;
    lab.type = OPERAND_LABEL;
    lab.label_name = strdup(str);
    return lab;
}

void gen_label(Bytecode *bytecode, Operand label) {
    Instr lab;
    lab.opcode = OPCODE_LABEL;
    lab.dest = label;
    array_append(*bytecode, lab);
}

Operand gen_call_instr_into(Bytecode *bytecode, Operand dest, Operand label) {
    Instr call;
    call.opcode = OPCODE_CALL;
    call.dest = dest;
    call.arg1 = label;
    array_append(*bytecode, call);
    return call.dest;
}

Operand gen_call_instr(Bytecode *bytecode, Operand label) {
    gen_call_instr_into(bytecode, (Operand){.type = OPERAND_INVALID}, label);
}

Operand gen_loadi_instr(Bytecode *bytecode, int imm) {
    Instr loadi;
    loadi.opcode = OPCODE_LOADI;
    loadi.dest.type = OPERAND_VREG;
    loadi.dest.vreg = vreg_count++;
    loadi.arg1.type = OPERAND_IMM;
    loadi.arg1.imm = imm;
    array_append(*bytecode, loadi);
    return loadi.dest;
}

Operand gen_load_instr(Bytecode *bytecode, int stack_offset) {
    Instr load;
    load.opcode = OPCODE_LOAD;
    load.dest.type = OPERAND_VREG;
    load.dest.vreg = vreg_count++;
    load.arg1.type = OPERAND_STACK;
    load.arg1.stack_offset = stack_offset;
    array_append(*bytecode, load);
    return load.dest;
}

Operand gen_store_instr(Bytecode *bytecode, int stack_offset, int vreg) {
    Instr store;
    store.opcode = OPCODE_STORE;
    store.dest.type = OPERAND_STACK;
    store.dest.vreg = stack_offset;
    store.arg1.type = OPERAND_VREG;
    store.arg1.vreg = vreg;
    array_append(*bytecode, store);
    return store.dest;
}

Operand gen_int_lit_bytecode(Bytecode *bytecode, AST *int_lit) {
    return gen_loadi_instr(bytecode, int_lit->int_lit);
}

Operand gen_expr_bytecode(Bytecode *bytecode, AST *expr) {
    if (expr->type == AST_INT_LIT) {
        return gen_int_lit_bytecode(bytecode, expr);
    }
    if (expr->type == AST_CALL) {
        Operand dest;
        dest.type = OPERAND_VREG;
        dest.vreg = vreg_count++;

        for (int i = 0; i < expr->arg_count; i++) {
            Operand dest = gen_expr_bytecode(bytecode, expr->args[i]);
            Instr param;
            param.opcode = OPCODE_PARAM;
            param.arg1 = dest;
            array_append(*bytecode, param);
        }

        return gen_call_instr_into(bytecode, dest, create_label_from(expr->callee->ident));
    }
    if (expr->type == AST_SYMBOL) {
        SymbolData *var = get_symbol(expr->symbol_table,
                                     expr->ident);
        return gen_load_instr(bytecode, var->stack_offset);
    }

    Instr instr;
    instr.dest.type = OPERAND_VREG;
    instr.dest.vreg = vreg_count++;
    instr.arg1 = gen_expr_bytecode(bytecode, expr->left);
    instr.arg2 = gen_expr_bytecode(bytecode, expr->right);

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

void gen_if_chain(Bytecode *bytecode, AST *ast, Operand end_label) {
    if (ast->type == AST_ELIF) {
        Operand expr = gen_expr_bytecode(bytecode, ast->condition);
        Operand label = gen_conditional_jump(bytecode, OPCODE_JMP_NEQ, create_label(), expr);
        gen_ast_bytecode(bytecode, ast->then);
        gen_jmp(bytecode, end_label);
        gen_label(bytecode, label);
        if (ast->otherwise) gen_if_chain(bytecode, ast->otherwise, end_label);
        return;
    } else if (ast->type == AST_ELSE) {
        gen_ast_bytecode(bytecode, ast->then);
        return;
    }
    assert(0 && "only elif and else should be chained in 'statement->otherwise' ");
}


void gen_procedure_bytecode(Procedure *procedure, AST *ast) {
    assert(ast->type == AST_PROCEDURE);

    procedure->name = ast->name->ident;
    procedure->param_count = ast->param_count;
    procedure->return_count = ast->return_count;
    stack_offset = 0;
    vreg_count = 0;

    for (int i = 0; i < ast->param_count; i++) {
        SymbolData *data = get_symbol(ast->symbols, ast->params[i]->symbol->ident);

        stack_offset += 4;
        data->stack_offset = stack_offset;
    }

    gen_ast_bytecode(&procedure->bytecode, ast->body);
    procedure->local_var_size = stack_offset;
    procedure->vreg_count = vreg_count;
}

void gen_program(Program *program, AST *ast) {
    for (int i = 0; i < ast->count; i++) {
        Procedure procedure = {0};
        gen_procedure_bytecode(&procedure, ast->items[i]);
        array_append(*program, procedure);
    }
}

void print_procedure(Procedure procedure) {
    printf("%s:\n", procedure.name);
    pretty_print_bytecode(procedure.bytecode);
}

void pretty_print_program(Program program) {
    for (int i = 0; i < program.count; i++) {
        print_procedure(program.items[i]);
        printf("\n");
    }
}

void gen_ast_bytecode(Bytecode *bytecode, AST *ast) {
    // Assumes AST is an AST_BLOCK which should be guaranteed by parsing and static analysis
    // Could probably change function name to indicate that fact though
    for (int i = 0; i < ast->count; i++) {
        AST *statement = ast->items[i];
        switch (statement->type) {
            case AST_PROCEDURE: {
                assert(0 && "Procedures shouldnt be inside other procedures");
            }
            case AST_BLOCK: {
                gen_ast_bytecode(bytecode, statement);
                break;
            }
            case AST_CALL: {
                for (int i = 0; i < statement->arg_count; i++) {
                    Operand dest = gen_expr_bytecode(bytecode, statement->args[i]);
                    SymbolData *var = get_symbol(statement->symbol->symbol_table,
                                                 statement->symbol->ident);

                    Instr param;
                    param.opcode = OPCODE_PARAM;
                    param.arg1 = dest;
                    array_append(*bytecode, param);
                }
                gen_call_instr(bytecode, create_label_from(statement->callee->ident));
                break;
            }
            case AST_DECL: {
                Operand expr = gen_expr_bytecode(bytecode, statement->expr);
                SymbolData *var = get_symbol(statement->symbol->symbol_table,
                                             statement->symbol->ident);
                stack_offset += 4;
                var->stack_offset = stack_offset;

                gen_store_instr(bytecode, var->stack_offset, expr.vreg);
                break;
            }
            case AST_ASSIGNMENT: {
                Operand expr = gen_expr_bytecode(bytecode, statement->expr);
                SymbolData *var = get_symbol(statement->symbol->symbol_table,
                                             statement->symbol->ident);
                gen_store_instr(bytecode, var->stack_offset, expr.vreg);
                break;
            }
            case AST_RETURN: {
                // TODO: in the future should probably have a return stack similar to function param stack
                // to support multiple return values
                Instr instr;
                instr.opcode = OPCODE_RET;
                instr.arg1 = gen_expr_bytecode(bytecode, statement->expr);
                array_append(*bytecode, instr);
                break;
            }
            case AST_IF: {
                Operand expr = gen_expr_bytecode(bytecode, statement->condition);
                Operand end_label = create_label();
                Operand next_cmp = gen_conditional_jump(bytecode, OPCODE_JMP_NEQ, create_label(), expr);

                gen_ast_bytecode(bytecode, statement->then);
                gen_jmp(bytecode, end_label);

                gen_label(bytecode, next_cmp);
                if (statement->otherwise) gen_if_chain(bytecode, statement->otherwise, end_label);
                gen_label(bytecode, end_label);
                break;
            }
            case AST_WHILE: {
                Operand begin_label = create_label();
                Operand end_label = create_label();

                gen_label(bytecode, begin_label);

                Operand expr = gen_expr_bytecode(bytecode, statement->condition);
                gen_conditional_jump(bytecode, OPCODE_JMP_NEQ, end_label, expr);

                gen_ast_bytecode(bytecode, statement->then);
                gen_jmp(bytecode, begin_label);
                gen_label(bytecode, end_label);

                break;
            }
            case AST_BINARY_OP:
                gen_expr_bytecode(bytecode, statement);
                break;
            default:
                assert(0 && "unsupported statement type");
        }
    }
}

void print_bytecode_op(Operand op) {
    if (op.type == OPERAND_IMM) {
        printf("%d", op.imm);
    } else if (op.type == OPERAND_VREG) {
        printf("r%d", op.vreg);
    } else if (op.type == OPERAND_LABEL) {
        printf(".%s", op.label_name);
    } else if (op.type == OPERAND_STACK) {
        printf("[rbp - %d]", op.stack_offset);
    } else {
        printf("%s", registers[op.reg]);
    }
}

void print_bytecode_operation(const char *name, Operand dst, Operand arg1, Operand arg2) {
    printf("\t");
    print_bytecode_op(dst);
    printf(" = %s ", name);
    print_bytecode_op(arg1);
    printf(", ");
    print_bytecode_op(arg2);
    printf("\n");
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
                print_bytecode_operation("add", instr.dest, instr.arg1, instr.arg2);
                break;
            case OPCODE_SUB:
                print_bytecode_operation("sub", instr.dest, instr.arg1, instr.arg2);
                break;
            case OPCODE_MUL:
                print_bytecode_operation("mul", instr.dest, instr.arg1, instr.arg2);
                break;
            case OPCODE_DIV:
                print_bytecode_operation("div", instr.dest, instr.arg1, instr.arg2);
                break;
            case OPCODE_CMP_EQ:
                print_bytecode_operation("c.eq", instr.dest, instr.arg1, instr.arg2);
                break;
            case OPCODE_CMP_LT:
                print_bytecode_operation("c.lt", instr.dest, instr.arg1, instr.arg2);
                break;
            case OPCODE_CMP_GT:
                print_bytecode_operation("c.gt", instr.dest, instr.arg1, instr.arg2);
                break;
            case OPCODE_CMP_LEQ:
                print_bytecode_operation("c.le", instr.dest, instr.arg1, instr.arg2);
                break;
            case OPCODE_CMP_GEQ:
                print_bytecode_operation("c.ge", instr.dest, instr.arg1, instr.arg2);
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
            case OPCODE_JMP:
                printf("\tjmp %s\n", instr.dest.label_name);
                break;
            case OPCODE_JMP_NEQ:
                printf("\tj.ne r%d, %s\n", instr.arg1.vreg, instr.dest.label_name);
                break;
            case OPCODE_PARAM:
                printf("\tparam r%d\n", instr.arg1.vreg);
                break;
            case OPCODE_CALL:
                if (instr.dest.type == OPERAND_INVALID)
                    printf("\tcall %s\n", instr.arg1.label_name);
                else
                    printf("\tr%d = call %s\n", instr.dest.vreg, instr.arg1.label_name);
                break;
            case OPCODE_PROC_BEGIN:
            case OPCODE_LABEL:
                printf("%s:\n", instr.dest.label_name);
                break;
            case OPCODE_PROC_END:
                printf("\n");
                break;
            default:
                printf("error unrecognized instruction in bytecode\n");
        }
    }
}

IntervalArray create_live_intervals_from_bytecode(Bytecode bytecode, int vreg_count) {
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


// NOTE: assumes intervals is already sorted
LocationArray linear_scan_register_allocation(IntervalArray *intervals, int vreg_count, PhysRegs *pregs) {
    LocationArray locations = {0};
    bool active_registers[] = { 1, 1, 1, 1, };
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
            active_registers[locations.items[active.items[j].vreg].register_index] = true;

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
                if (active_registers[k]) {
                    active_registers[k] = false;
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
