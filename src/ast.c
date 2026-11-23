#include <stdio.h>
#include <sys/_types/_sigaltstack.h>

#include "ast.h"
#include "common.h"
#include "error.h"
#include "symbol_table.h"

typedef enum {
    PRINT_TASK_NODE,
    PRINT_TASK_LIST,
    PRINT_TASK_TEXT,
    PRINT_TASK_INDENT,
    PRINT_TASK_NEWLINE,
    PRINT_TASK_DEINDENT,
} PrintTaskType;

typedef struct {
    PrintTaskType type;
    StringView str;
} Delimiter;

typedef struct {
    PrintTaskType type;
    union {
        StringView text;
        struct {
            ASTList list;
            Delimiter delim;
        };
        AST* node;
    };
} PrintTask;

typedef DEFINE_STACK(PrintTask) PrintTaskStack;
DEFINE_STACK_DEF(PrintTaskStack, PrintTask)

#define NODE_PRINT_TASK(n) (PrintTask) { .type = PRINT_TASK_NODE, .node = n }
#define LIST_PRINT_TASK(l, d) (PrintTask) { .type = PRINT_TASK_LIST, .list = l, .delim = d }
#define COMMA_DELIMITER (Delimiter) {.type = PRINT_TASK_TEXT, .str = SV_LITERAL(", ") }
#define NEWLINE_DELIMITER (Delimiter) { .type = PRINT_TASK_NEWLINE }
#define TEXT_PRINT_TASK(t) (PrintTask) { .type = PRINT_TASK_TEXT, .text = t }
#define NEWLINE_PRINT_TASK (PrintTask) { .type = PRINT_TASK_NEWLINE }
#define INDENT_PRINT_TASK (PrintTask) { .type = PRINT_TASK_INDENT }
#define DEINDENT_PRINT_TASK (PrintTask) { .type = PRINT_TASK_DEINDENT }

#define STR_CASE(unwrap, into, check, body) case check: { into *unwrap = CAST_AST(ast, into, check); body }

void ast_print(Arena *arena, AST *ast) {
    int mark = arena_mark(arena);
    StringBuilder sb = { 0 };
    string_builder_create(&sb, arena, 128 * sizeof(char));
    String out = ast_str(&sb, arena, ast);

    printf("%.*s\n", (int)out.len, out.data);
    arena_restore(arena, mark);
}

String ast_str(StringBuilder *sb, Arena *scratch, AST *ast) {
    if (ast == NULL) return (String) { 0 };

    size_t indent = 0;

    PrintTaskStack stack = (PrintTaskStack) { 0 };
    PrintTaskStack_set_backing(&stack, scratch);
    PrintTaskStack_push(&stack, NODE_PRINT_TASK(ast));

    while (!PrintTaskStack_empty(&stack)) {

        PrintTask cur = PrintTaskStack_pop(&stack);

        switch (cur.type) {
            case PRINT_TASK_NODE: {

                AST *ast = cur.node;

                switch (ast->type) {
                    STR_CASE(program, ASTProgram, AST_PROGRAM, {
                        PrintTaskStack_push(&stack, LIST_PRINT_TASK(program->decls, NEWLINE_DELIMITER));
                        break;
                    })
                    STR_CASE(proc, ASTProc, AST_PROCEDURE, {
                        if (proc->body)
                            PrintTaskStack_push(&stack, NODE_PRINT_TASK(proc->body));
                        else
                            PrintTaskStack_push(&stack, NEWLINE_PRINT_TASK);
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(") ")));
                        PrintTaskStack_push(&stack, LIST_PRINT_TASK(proc->rets, COMMA_DELIMITER));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(") => (")));
                        PrintTaskStack_push(&stack, LIST_PRINT_TASK(proc->params, COMMA_DELIMITER));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("(")));
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(proc->name));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("proc ")));
                        break;
                    })
                    STR_CASE(exter, ASTExtern, AST_EXTERN, {
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(exter->decl));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("extern ")));
                        break;
                    })
                    STR_CASE(block, ASTBlock, AST_BLOCK, {
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("}")));
                        PrintTaskStack_push(&stack, NEWLINE_PRINT_TASK);
                        PrintTaskStack_push(&stack, DEINDENT_PRINT_TASK);
                        PrintTaskStack_push(&stack, LIST_PRINT_TASK(block->stmts, NEWLINE_DELIMITER));
                        PrintTaskStack_push(&stack, NEWLINE_PRINT_TASK);
                        PrintTaskStack_push(&stack, INDENT_PRINT_TASK);
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("{")));
                        break;
                    })
                    STR_CASE(decl, ASTDecl, AST_DECL, {
                        if (decl->rhs != NULL) {
                            PrintTaskStack_push(&stack, NODE_PRINT_TASK(decl->rhs));
                            PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(" := ")));
                        } else {
                            PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(": type")));
                        }
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(decl->lhs));
                        break;
                    })
                    STR_CASE(assign, ASTAssign, AST_ASSIGNMENT, {
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(assign->rhs));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(" = ")));
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(assign->lhs));
                        break;
                    })
                    STR_CASE(_if, ASTIf, AST_IF, {
                        if (_if->otherwise) PrintTaskStack_push(&stack, NODE_PRINT_TASK(_if->otherwise));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("  ")));
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(_if->then));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(") ")));
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(_if->cond));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("if (")));
                        break;
                    })
                    STR_CASE(_if, ASTIf, AST_ELIF, {
                        if (_if->otherwise) PrintTaskStack_push(&stack, NODE_PRINT_TASK(_if->otherwise));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("  ")));
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(_if->then));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(") ")));
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(_if->cond));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("elif (")));
                        break;
                    })
                    STR_CASE(_if, ASTIf, AST_ELSE, {
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(_if->then));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("else ")));
                        break;
                    })
                    STR_CASE(_if, ASTWhile, AST_WHILE, {
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(_if->then));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(") ")));
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(_if->cond));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("while (")));
                        break;
                    })
                    STR_CASE(ret, ASTReturn, AST_RETURN, {
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(ret->expr));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("return ")));
                        break;
                    })
                    STR_CASE(call, ASTCall, AST_CALL, {
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(")")));
                        PrintTaskStack_push(&stack, LIST_PRINT_TASK(call->args, COMMA_DELIMITER));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("(")));
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(call->ident));
                        break;
                    })
                    STR_CASE(index, ASTIndex, AST_INDEX, {
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("]")));
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(index->index));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("[")));
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(index->head));
                        break;
                    })
                    STR_CASE(acc, ASTAccess, AST_ACCESS, {
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(acc->spec));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(".")));
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(acc->head));
                        break;
                    })
                    STR_CASE(ref, ASTRef, AST_REF, {
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(ref->head));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("&")));
                        break;
                    })
                    STR_CASE(bin, ASTBinaryOp, AST_BINARY_OP, {
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(bin->right));
                        switch (bin->op) {
                            case OP_LESS_THAN:      PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(" < "))); break;
                            case OP_EQUALS:         PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(" == "))); break;
                            case OP_GREATER_THAN:   PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(" > "))); break;
                            case OP_NEQUALS:        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(" != "))); break;
                            case OP_LESS_EQUALS:    PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(" <= "))); break;
                            case OP_GREATER_EQUALS: PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(" >= "))); break;
                            case OP_PLUS:           PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(" + "))); break;
                            case OP_MINUS:          PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(" - "))); break;
                            case OP_DIVIDE:         PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(" / "))); break;
                            case OP_MULTIPLY:       PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL(" * "))); break;
                            default: UNREACHABLE("invalid op");
                        }
                        PrintTaskStack_push(&stack, NODE_PRINT_TASK(bin->left));
                        break;
                    })
                    STR_CASE(list, ASTListConstruct, AST_LIST, {
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("}")));
                        PrintTaskStack_push(&stack, LIST_PRINT_TASK(list->items, COMMA_DELIMITER));
                        PrintTaskStack_push(&stack, TEXT_PRINT_TASK(SV_LITERAL("{")));
                        break;
                    })
                    STR_CASE(ident, ASTIdentifier, AST_IDENTIFIER, {
                        string_builder_cstr_append(sb, ident->name);
                        break;
                    })
                    STR_CASE(_int, ASTIntLit, AST_INT_LIT, {
                        string_builder_append(sb, arena_sprintf(scratch, "%u", _int->literal));
                        break;
                    })
                    STR_CASE(_float, ASTFloatLit, AST_FLOAT_LIT, {
                        string_builder_append(sb, arena_sprintf(scratch, "%g", (double)_float->literal));
                        break;
                    })
                    STR_CASE(_str, ASTStringLit, AST_STR_LIT, {
                        string_builder_sv_append(sb, _str->literal);
                        break;
                    })
                }

                break;
            }
            case PRINT_TASK_LIST: {

                ASTList list = cur.list;

                for (size_t i = 0; i < list.count; i++) {
                    if (i > 0) {
                        switch (cur.delim.type) {
                            case PRINT_TASK_TEXT:
                                if (cur.delim.str.len > 0) PrintTaskStack_push(&stack, TEXT_PRINT_TASK(cur.delim.str));
                                break;
                            case PRINT_TASK_NEWLINE:
                                PrintTaskStack_push(&stack, NEWLINE_PRINT_TASK);
                                break;
                            default:
                                break;
                        }
                    }
                    PrintTaskStack_push(&stack, NODE_PRINT_TASK(list.items[list.count - i - 1]));
                }
                
                break;
            }
            case PRINT_TASK_TEXT: {
                string_builder_sv_append(sb, cur.text);
                break;
            }
            case PRINT_TASK_NEWLINE: {
                string_builder_sv_append(sb, SV_LITERAL("\n"));
                string_builder_sv_append(sb, (StringView){.str = "\t\t\t\t\t\t\t\t\t\t", .len = indent});
                break;
            }
            case PRINT_TASK_INDENT: {
                indent++;
                break;
            }
            case PRINT_TASK_DEINDENT: {
                indent--;
                break;
            }
        }
    }

    string_builder_sv_append(sb, SV_LITERAL("\n"));
    return sb->string;
}
