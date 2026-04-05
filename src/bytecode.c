#include <stdio.h>

#include "ast.h"
#include "common.h"
#include "bytecode.h"
#include "symbol_table.h"

int vreg_count = 0;
int stack_offset = 0;

const char *registers[] = {
    "eax", "ecx", "edx", "edi",
};

const char *registers_8bit_low[] = { // NOTE: this is meant to be a temporary solution for accessing lower 8 bits for setCC instructions
    "al", "cl", "dl", "dil",
};

const char *registers_64bit[] = {
    "rax", "rcx", "rdx", "rdi"
};

// NOTE: in the future these instructions will hold more information like
// size of type, signed or unsigned, etc;

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
    return gen_call_instr_into(bytecode, (Operand){.type = OPERAND_INVALID}, label);
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

Operand gen_load_at_instr(Bytecode *bytecode, int stack_offset, int vreg) {
    Instr load;
    load.opcode = OPCODE_LOAD_INDEX;
    load.dest.type = OPERAND_VREG;
    load.dest.vreg = vreg_count++;
    load.arg1.type = OPERAND_STACK;
    load.arg1.stack_offset = stack_offset;
    load.arg2.type = OPERAND_VREG;
    load.arg2.vreg = vreg;
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

Operand emit_expr_bytecode(EmitContext *context, Bytecode *bytecode, AST *expr) {
    if (expr->type == AST_INT_LIT) {
        return gen_int_lit_bytecode(bytecode, expr);
    }
    if (expr->type == AST_LIST) {
        return (Operand){0};
    }
    if (expr->type == AST_INDEX) {
        SymbolData *arr = get_symbol(context->scope, expr->access->ident);
        Operand at = emit_expr_bytecode(context, bytecode, expr->index);
    
        return gen_load_at_instr(bytecode, arr->stack_offset, at.vreg);
    }
    if (expr->type == AST_CALL) {
        Operand dest;
        dest.type = OPERAND_VREG;
        dest.vreg = vreg_count++;

        for (int i = 0; i < expr->arg_count; i++) {
            Operand dest = emit_expr_bytecode(context, bytecode, expr->args[i]);
            Instr param;
            param.opcode = OPCODE_PARAM;
            param.arg1 = dest;
            array_append(*bytecode, param);
        }

        return gen_call_instr_into(bytecode, dest, create_label_from(expr->callee->ident));
    }
    if (expr->type == AST_SYMBOL) {
        SymbolData *var = get_symbol(context->scope, expr->ident);
        return gen_load_instr(bytecode, var->stack_offset);
    }

    Instr instr;
    instr.dest.type = OPERAND_VREG;
    instr.dest.vreg = vreg_count++;
    instr.arg1 = emit_expr_bytecode(context, bytecode, expr->left);
    instr.arg2 = emit_expr_bytecode(context, bytecode, expr->right);

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


void emit_if_chain(EmitContext *context, Bytecode *bytecode, AST *ast, Operand end_label) {
    if (ast->type == AST_ELIF) {
        Operand expr = emit_expr_bytecode(context, bytecode, ast->condition);
        Operand label = gen_conditional_jump(bytecode, OPCODE_JMP_NEQ, create_label(), expr);

        emit_statement_bytecode(context, bytecode, ast->then);

        gen_jmp(bytecode, end_label);
        gen_label(bytecode, label);
        if (ast->otherwise) emit_if_chain(context, bytecode, ast->otherwise, end_label);
        return;
    } else if (ast->type == AST_ELSE) {
        emit_block_bytecode(context, bytecode, ast->then);
        return;
    }
    assert(0 && "only elif and else should be chained in 'statement->otherwise' ");
}


void emit_program_bytecode(EmitContext *context, Program *program, AST *ast) {
    for (int i = 0; i < ast->count; i++) {
        Procedure procedure = {0};
        switch (ast->items[i]->type) {
            case AST_PROCEDURE:
                emit_procedure_bytecode(context, &procedure, ast->items[i]);
                array_append(*program, procedure);
                break;
            case AST_EXTERN:
                break;
            default:
                UNREACHABLE("not top-level decl");
                break;
        }
    }
}


void emit_procedure_bytecode(EmitContext *context, Procedure *procedure, AST *ast) {
    assert(ast->type == AST_PROCEDURE);

    procedure->name = ast->name->ident;
    procedure->param_count = ast->param_count;
    procedure->return_count = ast->return_count;
    stack_offset = 0;
    vreg_count = 0;

    context->scope = ast->symbols;
    for (int i = 0; i < ast->param_count; i++) {
        SymbolData *data = get_symbol(context->scope, ast->params[i]->symbol->ident);

        stack_offset += 4;
        data->stack_offset = stack_offset;
    }
    emit_block_bytecode(context, &procedure->bytecode, ast->body);
    context->scope = ast->symbols->prev;

    procedure->local_var_size = stack_offset;
    procedure->vreg_count = vreg_count;
}


void emit_decl_bytecode(EmitContext *context, Bytecode *bytecode, AST *decl) {
    SymbolData *var = get_symbol(context->scope, decl->symbol->ident);

    if (decl->qtype->type == QUECTO_ARRAY) {
        var->stack_offset = stack_offset + 4;
        for (int i = 0; i < decl->qtype->array_size; i++) {
            stack_offset += 4;
            Operand expr = emit_expr_bytecode(context, bytecode, decl->expr->items[i]);
            gen_store_instr(bytecode, stack_offset, expr.vreg);   
        }
    } else {
        stack_offset += 4;
        var->stack_offset = stack_offset;

        Operand expr = emit_expr_bytecode(context, bytecode, decl->expr);
        gen_store_instr(bytecode, stack_offset, expr.vreg);
    }
}


void emit_call_bytecode(EmitContext *context, Bytecode *bytecode, AST *call) {
    for (int i = 0; i < call->arg_count; i++) {
        Operand dest = emit_expr_bytecode(context, bytecode, call->args[i]);
        Instr param;

        param.opcode = OPCODE_PARAM;
        param.arg1 = dest;
        array_append(*bytecode, param);
    }
    gen_call_instr(bytecode, create_label_from(call->callee->ident));
}


void emit_statement_bytecode(EmitContext *context, Bytecode *bytecode, AST *statement) {
    switch (statement->type) {
        case AST_CALL:
            emit_call_bytecode(context, bytecode, statement);
            break;
        case AST_BLOCK:
            emit_block_bytecode(context, bytecode, statement);
            break;
        case AST_DECL:
            emit_decl_bytecode(context, bytecode, statement);
            break;
        case AST_ASSIGNMENT:
            emit_assign_bytecode(context, bytecode, statement);
            break;
        case AST_RETURN:
            emit_return_bytecode(context, bytecode, statement);
            break;
        case AST_IF:
            emit_if_bytecode(context, bytecode, statement);
            break;
        case AST_WHILE:
            emit_while_bytecode(context, bytecode, statement);
            break;
        case AST_BINARY_OP:
            emit_expr_bytecode(context, bytecode, statement);
            break;
        default:
            UNREACHABLE("ASTType");
    }
}

void emit_assign_bytecode(EmitContext *context, Bytecode *bytecode, AST *assignment) {
    Operand expr = emit_expr_bytecode(context, bytecode, assignment->expr);
    SymbolData *var = get_symbol(context->scope, assignment->symbol->ident);

    gen_store_instr(bytecode, var->stack_offset, expr.vreg);
}

void emit_return_bytecode(EmitContext *context, Bytecode *bytecode, AST *ret) {
    // TODO: in the future should probably have a return stack similar to function param stack
    // to support multiple return values
    Instr instr;
    instr.opcode = OPCODE_RET;
    instr.arg1 = emit_expr_bytecode(context, bytecode, ret->expr);
    array_append(*bytecode, instr);
}

void emit_if_bytecode(EmitContext *context, Bytecode *bytecode, AST *ifs) {
    Operand expr = emit_expr_bytecode(context, bytecode, ifs->condition);
    Operand end_label = create_label();
    Operand next_cmp = gen_conditional_jump(bytecode, OPCODE_JMP_NEQ, create_label(), expr);

    emit_statement_bytecode(context, bytecode, ifs->then);
    gen_jmp(bytecode, end_label);

    gen_label(bytecode, next_cmp);
    if (ifs->otherwise) emit_if_chain(context, bytecode, ifs->otherwise, end_label);
    gen_label(bytecode, end_label);
}

void emit_while_bytecode(EmitContext *context, Bytecode *bytecode, AST *whiles) {
    Operand begin_label = create_label();
    Operand end_label = create_label();

    gen_label(bytecode, begin_label);

    Operand expr = emit_expr_bytecode(context, bytecode, whiles->condition);
    gen_conditional_jump(bytecode, OPCODE_JMP_NEQ, end_label, expr);

    emit_statement_bytecode(context, bytecode, whiles->then);
    gen_jmp(bytecode, begin_label);
    gen_label(bytecode, end_label);
}


void emit_block_bytecode(EmitContext *context, Bytecode *bytecode, AST *ast) {
    for (int i = 0; i < ast->count; i++) {
        emit_statement_bytecode(context, bytecode, ast->items[i]);
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


void print_live_intervals(IntervalArray intervals) {
    for (int i = 0; i < intervals.count; i++) {
        printf("r%d:\t[%d, %d)\n", intervals.items[i].vreg, intervals.items[i].start, intervals.items[i].end);
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
            case OPCODE_LOAD_INDEX:
                printf("\tr%d = [bp - %d + r%d]\n", instr.dest.vreg, instr.arg1.stack_offset, instr.arg2.vreg);
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
            case OPCODE_LABEL:
                printf("%s:\n", instr.dest.label_name);
                break;
            default:
                printf("error unrecognized instruction in bytecode\n");
        }
    }
}
