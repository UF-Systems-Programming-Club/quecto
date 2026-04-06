#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "symbol_table.h"
#include "tokenizer.h"
#include "parser.h"
#include "ast.h"
#include "analysis.h"
#include "bytecode.h"
#include "codegen.h"
#include "error.h"
#include "backends/linux_x64.h"

#ifdef __MACH__
#define EXIT_STATUS "0x2000001"
#define ENTRY_SYMBOL "_main"
#else
#define EXIT_STATUS "60"
#define ENTRY_SYMBOL "_start"
#endif

void help_prompt();
int compile(const char *filename);

int main(int argc, char **argv) { // quecto <command> <input> {flags}
    if (argc == 1) {
        help_prompt();
    } else if (argc >= 2) {
        int current_arg = 1;
        if (strncmp(argv[current_arg++], "build", sizeof("build")) == 0) {
            if (current_arg >= argc) {
                printf("no file supplied.");
                return -1;
            }
            char *filename = argv[current_arg];
            if (compile(filename) == 0) {
                printf("compiled successfully.\n");
            } else {
                printf("error\n");
            }
        } else if (strncmp(argv[1], "help", sizeof("help")) == 0) {
            help_prompt();
        }
    }   
}

void help_prompt() {
    printf("info: Usage: quecto [command] [options]\n");
    printf("List of Commands:\n");
    printf("\tbuild {file}\t\tBuilds the file.\n");
    printf("\thelp\t\tShows this prompt.\n");
}

int compile(const char *filename) {
    FILE *f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    src = malloc(fsize + 1);
    fread(src, 1, fsize, f);
    fclose(f);

    src[fsize] = '\0';
    src_size = fsize;
    
    TokenArray tokens = tokenize(src, src_size);
    if (error) return -1;

    Arena backing = {0};
    HashTable types = {0}; // intern table

    arena_create(&backing, 1024 * 1024);

    ParserState parser = {
        .tokens = tokens,
        .arena = &backing,
        .type_intern_table = &types,
    };

    AST *ast = parse_program(&parser);

    if (error) return -1;

    SymbolTable symbols = { 0 };

    AnalysisContext analysis_ctx = (AnalysisContext) {
        .arena = &backing,
        .type_intern_table = &types,
        .symbol_table = &symbols,
    };

    analyze_ast(&analysis_ctx, ast);

    print_ast(ast, 0);

    EmitContext emit_ctx = (EmitContext) {
        .scope = &symbols,
    };

    Program program = {0};
    emit_program_bytecode(&emit_ctx, &program, ast);

    pretty_print_program(program);
    
    PhysRegs pregs = {0};
    adhere_program(&program, &pregs);
    analyze_program(&program, &pregs);

    compile_program_with(&LINUX_X86_64_BACKEND, &program);

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
    
    arena_free(&backing);
    if (error) return -1;
    return 0;
}
