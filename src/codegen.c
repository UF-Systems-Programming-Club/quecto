#include <assert.h>
#include <stdio.h>

#include "AST.h"
#include "codegen.h"

int stack_offset = 0;
const char *register_list[][4] = {
    [BIT_8]  = { "al",  "bl",  "cl",  "dl" },
    [BIT_16] = { "ax",  "bx",  "cx",  "dx", },
    [BIT_32] = { "eax", "ebx", "ecx", "edx" },
    [BIT_64] = { "rax", "rbx", "rcx", "rdx" }
};

bool register_free_list[] = { 1, 1, 1, 1 };

int current_symbol = 0;
LocTable loc_table = {0};

// TODO: evict register should evict to another register if possible before stack
int evict_register(FILE *file, int reg) {
}

int allocate_register(FILE *file) {
    for (int i = 0; i < (int)(sizeof(register_free_list) / sizeof(register_free_list[0])); i++) {
        if (register_free_list[i]) {
            register_free_list[i] = 0;
            return i;
        }
    }

    assert(0);
    return -1;
}

void free_register(int reg) {
    register_free_list[reg] = 0;
}

int load_symbol_into_register(FILE *file, Symbol sym) {
    int reg;
    switch (loc_table.locs[sym].type) {
        case LOC_REGISTER:
            reg = loc_table.locs[sym].register_index;
            break;
        case LOC_STACK:
            assert(0);
            break;
        case LOC_IMMEDIATE:
            reg = allocate_register(file);
            fprintf(file, "\tmov\t%s, %u\n", register_list[BIT_32][reg], loc_table.locs[sym].value);
            loc_table.locs[sym].type = LOC_REGISTER;
            loc_table.locs[sym].register_index = reg;
            break;
    }
    return reg;
}


Symbol generate_add(FILE *file, Symbol sym1, Symbol sym2) {
    // TODO: this is really gross, need to add some abstracting functions for the location table
    int reg = load_symbol_into_register(file, sym1);

    switch (loc_table.locs[sym2].type) {
        case LOC_REGISTER:
            fprintf(file,
                    "\tadd\t%s, %s\n",
                    register_list[BIT_32][reg],
                    register_list[BIT_32][loc_table.locs[sym2].register_index]);
            break;
        case LOC_STACK:
            assert(0);
            break;
        case LOC_IMMEDIATE:
            fprintf(file, "\tadd\t%s, %u\n", register_list[BIT_32][reg], loc_table.locs[sym2].value);
            break;
    }

    Symbol res = current_symbol++;
    loc_table.locs[res].type = LOC_REGISTER;
    loc_table.locs[res].register_index = reg;
    return res;
}

Symbol generate_subtract(FILE *file, Symbol sym1, Symbol sym2) {
    // TODO: this is really gross, need to add some abstracting functions for the location table
    int reg = load_symbol_into_register(file, sym1);

    switch (loc_table.locs[sym2].type) {
        case LOC_REGISTER:
            fprintf(file,
                    "\tsub\t%s, %s\n",
                    register_list[BIT_32][reg],
                    register_list[BIT_32][loc_table.locs[sym2].register_index]);
            break;
        case LOC_STACK:
            assert(0);
            break;
        case LOC_IMMEDIATE:
            fprintf(file, "\tsub\t%s, %u\n", register_list[BIT_32][reg], loc_table.locs[sym2].value);
            break;
    }

    Symbol res = current_symbol++;
    loc_table.locs[res].type = LOC_REGISTER;
    loc_table.locs[res].register_index = reg;
    return res;
}

Symbol generate_multiply(FILE *file, Symbol sym1, Symbol sym2) {
    // TODO: this is really gross, need to add some abstracting functions for the location table
    int reg = load_symbol_into_register(file, sym1);

    switch (loc_table.locs[sym2].type) {
        case LOC_REGISTER:
            fprintf(file,
                    "\timul\t%s, %s\n",
                    register_list[BIT_32][reg],
                    register_list[BIT_32][loc_table.locs[sym2].register_index]);
            break;
        case LOC_STACK:
            assert(0);
            break;
        case LOC_IMMEDIATE:
            fprintf(file, "\timul\t%s, %u\n", register_list[BIT_32][reg], loc_table.locs[sym2].value);
            break;
    }

    Symbol res = current_symbol++;
    loc_table.locs[res].type = LOC_REGISTER;
    loc_table.locs[res].register_index = reg;
    return res;
}

Symbol generate_divide(FILE *file, Symbol sym1, Symbol sym2) {
    // TODO: in order to implement division we need to implement evicting registers onto the stack
    assert(0);
    // fprintf(file, "\tdiv\t%s, %s\n", register_list[BIT_32][reg1], register_list[BIT_32][reg2]);
}

Symbol generate_ast_assembly(FILE *file, AST *ast) {
    assert(ast->type != AST_FLOAT_LIT);

    if (ast->type == AST_INT_LIT) {
        Loc loc = {
            .type = LOC_IMMEDIATE,
            .value = ast->int_lit
        };
        loc_table.locs[current_symbol++] = loc;

        return current_symbol - 1;
    }

    switch (ast->type) {
        case AST_BINARY_OP:
        {
            Symbol left = generate_ast_assembly(file, ast->left);
            Symbol right = generate_ast_assembly(file, ast->right);

            Symbol res;
            switch (ast->op) {
                case OP_PLUS:
                    res = generate_add(file, left, right);
                    return res;
                case OP_MINUS:
                    res = generate_subtract(file, left, right);
                    return res;
                case OP_MULTIPLY:
                    res = generate_multiply(file, left, right);
                    return res;
                case OP_DIVIDE:
                    res = generate_divide(file, left, right);
                    return res;
            }
            break;
        }
        case AST_DECLARATION:
            break;
        case AST_RETURN:
            break;
        case AST_FLOAT_LIT:
        case AST_INT_LIT:
            break;
    }

}
