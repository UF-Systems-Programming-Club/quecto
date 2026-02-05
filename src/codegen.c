#include "codegen.h"

const char *register_list[][4] = {
    [BITS_8]  = { "al",  "bl",  "cl",  "dl" },
    [BITS_16] = { "ax",  "bx",  "cx",  "dx", },
    [BITS_32] = { "eax", "ebx", "ecx", "edx" },
    [BITS_64] = { "rax", "rbx", "rcx", "rdx" }
};

bool register_free_list[] = { 0, 0, 0, 0 };

int allocate_register() {
    for (int i = 0; i < (int)(sizeof(register_free_list) / sizeof(register_free_list[0])); i++) {
        if (register_free_list[i] == 0) {
            register_free_list[i] = 1;
            return i;
        }
    }
    return -1;
}

void free_register(int reg) {
    register_free_list[reg] = 0;
}

void generate_load_register_with_int(FILE *file, int reg, int val) {
    // TODO: Implementation for loading int into register
}

void generate_add_registers(FILE *file, int reg1, int reg2) {
    // TODO: Implementation for adding registers
}

Loc generate_ast_assembly(FILE *file, AST *ast) {
    if (ast->type == AST_INT_LIT) {
        int reg = allocate_register();
        // Note: renamed from load_register_with_int to match your new prototype
        generate_load_register_with_int(file, reg, ast->int_lit);
        Loc loc = {
            .type = LOC_REGISTER,
            .register_index = reg
        };
        return loc;
    }

    Loc loc_left = generate_ast_assembly(file, ast->left);
    Loc loc_right = generate_ast_assembly(file, ast->right);

    switch (ast->op) {
        case OP_PLUS:     break;
        case OP_MINUS:    break;
        case OP_MULTIPLY: break;
        case OP_DIVIDE:   break;
    }
    
    return (Loc){0};
}