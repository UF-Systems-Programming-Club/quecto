#include <stdio.h>
#include <stdlib.h>

#include "tokenizer.h"
#include "parser.h"
#include "ast.h"
#include "analysis.h"
#include "bytecode.h"
#include "codegen.h"
#include "error.h"

#ifdef __MACH__
#define EXIT_STATUS "0x2000001"
#define ENTRY_SYMBOL "_main"
#else
#define EXIT_STATUS "60"
#define ENTRY_SYMBOL "_start"
#endif

int main(int argc, char **argv) {
    FILE *f = fopen("examples/proc.q", "r");

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);

    fseek(f, 0, SEEK_SET);

    src = malloc(fsize + 1);
    fread(src, 1, fsize, f);
    fclose(f);

    src[fsize] = '\0';
    src_size = fsize;

    // printf("%s\n", src);

    TokenArray tokens = tokenize(src, src_size);
    if (error) return 0;

    // for (int i = 0; i < tokens.count; i++) {
    //     print_token(tokens.items[i]);
    // }
    // printf("\n");

    Arena ast_arena;
    arena_create(&ast_arena, 1024 * 1024);

    ParserState parser = {0};
    parser.tokens = tokens;
    parser.arena = &ast_arena;
    // global symbol table initialization
    parser.cur_symbol_table = arena_alloc_type(&ast_arena, SymbolTable);

    AST *ast = parse_program(&parser);

    print_ast(ast, 0);
    printf("\n");

    analyze_ast(ast);

    print_ast(ast, 0);
    printf("\n");

    if (error) return 0;

    Program program = {0};
    emit_program_bytecode(&program, ast);
    pretty_print_program(program);

    PhysRegs pregs = {0};
    adhere_program(&program, &pregs);
    analyze_program(&program, &pregs);



    FILE *out = fopen("out.S", "w");

    fprintf(out, "\tglobal\t" ENTRY_SYMBOL "\n\n");
    fprintf(out, "\tsection\t.text\n");
    fprintf(out, ENTRY_SYMBOL ":\n");
    fprintf(out, "\tcall\tmain\n");
    fprintf(out, "\tmov\tedi, eax\n");
    fprintf(out, "\tmov\teax, " EXIT_STATUS "\n");
    fprintf(out, "\tsyscall\n\n");

    compile_program(out, &program);

    fclose(out);
    arena_free(&ast_arena);
}
