#include <stdio.h>
#include <stdlib.h>

#include "tokenizer.h"
#include "parser.h"
#include "ast.h"
#include "ir.h"
#include "codegen.h"

#ifdef __MACH__
#define EXIT_STATUS "0x2000001"
#define ENTRY_SYMBOL "_main"
#else
#define EXIT_STATUS "60"
#define ENTRY_SYMBOL "_start"
#endif

int main(int argc, char **argv) {
    FILE *f = fopen("hello.q", "r");

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);

    fseek(f, 0, SEEK_SET);

    char *buf = (char *)malloc(fsize + 1);
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

    ParserState parser = {0};
    parser.tokens = tokens;
    // global symbol table initialization
    parser.cur_symbol_table = calloc(1, sizeof(SymbolTable));

    AST *ast = parse_program(&parser);

    print_symbol_table(parser.cur_symbol_table, 0);
    printf("\n");

    if (!parser.error) {
        print_ast(ast, 0);
        printf("\n");

        InstList ir = {0};
        generate_ir_from_ast(&ir, ast);
        pretty_print_ir(ir);
        printf("\n");

        ir = adhere_ir_to_machine_spec(ir);
        pretty_print_ir(ir);
        printf("\n");

        IntervalArray intervals = create_live_intervals_from_ir(ir);
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

        LocationArray locations = linear_scan_register_allocation(&intervals);
        for (int i = 0; i < locations.count; i++) {
            printf("r%d @ %s\n", i, registers[locations.items[i].register_index]);
        }

        FILE *out = fopen("out.S", "w");

        fprintf(out, "\tglobal\t" ENTRY_SYMBOL "\n\n");
        fprintf(out, "\tsection\t.text\n");
        fprintf(out, ENTRY_SYMBOL ":\n");
        fprintf(out, "\tmov\trbp, rsp\n");

        generate_assembly_from_ir(out, ir, locations);

        fprintf(out, "\tmov\tedi, %d\n", 0);
        fprintf(out, "\tmov\teax, " EXIT_STATUS "\n");
        fprintf(out, "\tsyscall\n");

        fclose(out);
    }
}
