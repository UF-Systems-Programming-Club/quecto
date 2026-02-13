#include <stdio.h>
#include <stdlib.h>

#include "tokenizer.h"
#include "parser.h"
#include "AST.h"
#include "codegen.h"

#ifdef __MACH__
#define EXIT_STATUS "0x2000001"
#define ENTRY_SYMBOL "_main"
#else
#define EXIT_STATUS "60"
#define ENTRY_SYMBOL "_start"
#endif

int main() {
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

    AST *ast = parse_program(&parser);

    if (!parser.error) {

        print_ast(ast);

        FILE *out = fopen("out.S", "w");

        fprintf(out, "\tglobal\t" ENTRY_SYMBOL "\n\n");
        fprintf(out, "\tsection\t.text\n");
        fprintf(out, ENTRY_SYMBOL ":\n");

        Symbol sym = generate_ast_assembly(out, ast);

        fprintf(out, "\tmov\tedi, %s\n", register_list[BIT_32][loc_table.locs[sym].register_index]);
        fprintf(out, "\tmov\teax, " EXIT_STATUS "\n");
        fprintf(out, "\tsyscall\n");

        fclose(out);
    }
}
