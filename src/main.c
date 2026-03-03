#include <stdio.h>
#include <stdlib.h>

#include "tokenizer.h"
#include "parser.h"
#include "ast.h"
#include "bytecode.h"
#include "codegen.h"

#ifdef __MACH__
#define EXIT_STATUS "0x2000001"
#define ENTRY_SYMBOL "_main"
#else
#define EXIT_STATUS "60"
#define ENTRY_SYMBOL "_start"
#endif

int main(int argc, char **argv) {
    FILE *f = fopen("examples/hello.q", "r");

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);

    fseek(f, 0, SEEK_SET);

    char *buf = malloc(fsize + 1);
    fread(buf, 1, fsize, f);
    fclose(f);

    buf[fsize] = '\0';
    size_t buf_size = fsize;

    printf("%s\n", buf);

    TokenArray tokens = tokenize(buf, buf_size);

    for (int i = 0; i < tokens.count; i++) {
        print_token(tokens.items[i]);
    }
    printf("\n");

    Arena ast_arena;
    arena_create(&ast_arena, 1024 * 1024);

    ParserState parser = {0};
    parser.tokens = tokens;
    parser.arena = &ast_arena;
    // global symbol table initialization
    parser.cur_symbol_table = arena_alloc_type(&ast_arena, SymbolTable);

    AST *ast = parse_program(&parser);

    if (!parser.error) {
        print_symbol_table(parser.cur_symbol_table, 0);
        printf("\n");

        print_ast(ast, 0);
        printf("\n");

        Bytecode bytecode = {0};
        gen_bytecode_from_ast(&bytecode, ast);
        pretty_print_bytecode(bytecode);
        printf("\n");

        PhysRegs pregs = {0};
        bytecode = adhere_bytecode_to_machine_spec(bytecode, &pregs);
        pretty_print_bytecode(bytecode);
        printf("\n");

        IntervalArray intervals = create_live_intervals_from_bytecode(bytecode);
        // Bubble sort
        for (int i = 0; i < intervals.count - 1; i++) {
            bool swapped = false;
            for (int j = 0; j < intervals.count - i - 1; j++) {
                if (intervals.items[j].start > intervals.items[j+1].start) {
                    Interval temp = intervals.items[j];
                    intervals.items[j] = intervals.items[j+1];
                    intervals.items[j+1] = temp;
                    swapped = true;
                }
            }
            if (!swapped)
                break;
        }

        print_live_intervals(intervals);
        printf("\n");

        LocationArray locations = linear_scan_register_allocation(&intervals, &pregs);
        for (int i = 0; i < locations.count; i++) {
            printf("r%d @ %s\n", i, registers[locations.items[i].register_index]);
        }

        FILE *out = fopen("out.S", "w");

        fprintf(out, "\tglobal\t" ENTRY_SYMBOL "\n\n");
        fprintf(out, "\tsection\t.text\n");
        fprintf(out, ENTRY_SYMBOL ":\n");
        fprintf(out, "\tcall\tmain\n");
        fprintf(out, "\tmov\tedi, eax\n");
        fprintf(out, "\tmov\teax, " EXIT_STATUS "\n");
        fprintf(out, "\tsyscall\n\n");


        fprintf(out, "main:\n");
        fprintf(out, "\tmov\trbp, rsp\n");
        generate_assembly_from_bytecode(out, bytecode, locations);

        fclose(out);
    }
    arena_free(&ast_arena);
}
