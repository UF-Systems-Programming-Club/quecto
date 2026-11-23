#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "ir.h"
#include "symbol_table.h"
#include "tokenizer.h"
#include "parser.h"
#include "ast.h"
#include "analysis.h"
#include "emission.h"
#include "codegen.h"
#include "passes.h"
#include "error.h"
#include "backends/linux_x64.h"

typedef enum {
    DEBUG_PRINT_TOKENS = 1,
    DEBUG_PRINT_AST = 2,
    DEBUG_PRINT_CFG = 4,
} BuildFlags;

typedef struct {
    const char *input_file;
    const char *output_file;
    BuildFlags flags;
} BuildOptions;

typedef enum {
    COMMAND_UNRECOGNIZED = -1,
    COMMAND_HELP = 0,
    COMMAND_BUILD
} CommandKind;

typedef struct {
    CommandKind kind;
    BuildOptions build_options;
} Command;


void help_prompt(void);
void unrecognized_prompt(void);
int compile(BuildOptions options);

int main(int argc, char **argv) { // quecto <command> <input> {flags}

    Command command = (Command) {
        .kind = COMMAND_HELP,
        .build_options = {
            .input_file = NULL,
            .output_file = "out.S",
            .flags = 0,
        }
    };

    if (argc >= 1) {

        #define CONST_STR_CMP(a, b) (strncmp(a, b, sizeof(b)) == 0)

        if (CONST_STR_CMP(argv[1], "build")) {
            command.kind = COMMAND_BUILD;
        } else if (CONST_STR_CMP(argv[1], "help")) {
            command.kind = COMMAND_HELP;
        }

        for (int i = 2; i < argc;) {
            if (CONST_STR_CMP(argv[i], "-o")) {
                command.build_options.output_file = argv[i+1];
                i += 2;
            } else if (CONST_STR_CMP(argv[i], "-i")) {
                command.build_options.input_file = argv[i+1];
                i += 2;
            } else if (CONST_STR_CMP(argv[i], "--dump-ast")) {
                command.build_options.flags |= DEBUG_PRINT_AST;
                i++;
            } else if (CONST_STR_CMP(argv[i], "--dump-tokens")) {
                command.build_options.flags |= DEBUG_PRINT_TOKENS;
                i++;
            }
        }

        #undef CONST_STR_CMP

    }
    
    // run command

    switch (command.kind) {
        case COMMAND_UNRECOGNIZED:
            unrecognized_prompt();
            break;
        case COMMAND_HELP:
            help_prompt();
            break;
        case COMMAND_BUILD:
            if (command.build_options.input_file != NULL) {
                if (compile(command.build_options) == 0) {
                    printf("compiled successfully.\n");
                } else {
                    printf("error");
                    return -1;
                }
            } else {
                printf("no file supplied");
                return -1;
            }
            break;
    }
}

void help_prompt(void) {
    printf("info: Usage: quecto [command] [options]\n");
    printf("List of Commands:\n");
    printf("\tbuild {file} {-i input} {-o output}\t\tBuilds the file.\n");
    printf("\thelp\t\tShows this prompt.\n");
}

void unrecognized_prompt(void) {
    printf("unrecognized command\n");
}

typedef String (DumpFn(Arena *scratch, const void *context));

String generate_ast_dump(Arena *scratch, const void *context) {
    AST *ast = *(AST * const *) context;
    StringBuilder sb = {0};
    string_builder_create(&sb, scratch, 128);
    return ast_str(&sb, scratch, ast);
}

String generate_token_dump(Arena *scratch, const void *context) {
    const TokenArray *tokens = context;
    return tk_array_str(scratch, *tokens);
}

void dump_intermediate_file(Arena *scratch, const char *file, StringView suffix, DumpFn generator, const void *context) {
    size_t mark = arena_mark(scratch);

    StringBuilder sb = { 0 };
    string_builder_create(&sb, scratch, sizeof(char) * 16);
    string_builder_cstr_append(&sb, file);
    string_builder_sv_append(&sb, suffix);

    String contents = generator(scratch, context);

    const char *filename = string_builder_to_cstr(&sb, scratch);
    
    FILE *out = fopen(filename, "w");
    fwrite(contents.data, 1, contents.len, out);
    fclose(out);
    
    arena_restore(scratch, mark);
}

int compile(BuildOptions options) {
    FILE *f = fopen(options.input_file, "r");
    
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    src = malloc(fsize + 1);
    fread(src, 1, fsize, f);
    fclose(f);

    src[fsize] = '\0';
    src_size = fsize;

    Arena backing = {0};
    Arena scratch = {0};
    Arenas arena = {
        .scratch = &scratch,
        .persistent = &backing
    };

    arena_create(&backing, 1024 * 2048);
    arena_create(&scratch, 1024 * 1024);
    
    TokenArray tokens = tokenize(&arena, src, src_size);

    if ((options.flags & DEBUG_PRINT_TOKENS) == DEBUG_PRINT_TOKENS) {
        dump_intermediate_file(&scratch, options.output_file, SV_LITERAL(".tk"),
                generate_token_dump, &tokens);
    }

    if (error) return -1;

    HashTable types = { 0 }; // intern table

    ParserState parser = {
        .tokens = tokens,
        .arena = &backing,
        .type_intern_table = &types,
    };

    AST *ast = parse_program(&parser);

    if (error) return -1;

    if ((options.flags & DEBUG_PRINT_AST) == DEBUG_PRINT_AST) {
        dump_intermediate_file(&scratch, options.output_file, SV_LITERAL(".ast"),
                generate_ast_dump, &ast);
    }

    SymbolTable symbols = { 0 };

    AnalysisContext analysis_ctx = (AnalysisContext) {
        .arena = &backing,
        .type_intern_table = &types,
        .symbol_table = &symbols,
    };

    analyze_ast(&analysis_ctx, ast);

    EmitContext emit_ctx = (EmitContext) {
        .scope = &symbols,
        .arena = &backing,
    };

    Passes passes = (PassFn[]) {
            pass_insert_phis,
            pass_rename,
            pass_kill_slots,
            pass_remove_copies,
            pass_liveness,
            pass_compute_liveness,
            pass_crosses_call,
            pass_precoloring,
            pass_color_cfg,
            pass_phis_into_copies,
            pass_sweep_nops,
            debug_pass_print_colored,
            NULL,
    };

    Program program = { .symbols = &symbols };
    emit_program(&emit_ctx, &program, ast);

    passes_run(&program, passes, (Arenas) { .scratch = &scratch, .persistent = &backing});

    FILE *out = fopen(options.output_file, "w");
    compile_program_with(out, &scratch, &LINUX_X86_64_BACKEND, &program);

    fclose(out);
    
    arena_free(&backing);
    if (error) return -1;
    return 0;
}
