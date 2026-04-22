#ifndef IR_H
#define IR_H

#include "ast.h"
#include "symbol_table.h"

typedef enum {
    PR_BP,
} PhysicalReg;

typedef enum {
    OPCODE_ADD,
    OPCODE_SUB,
    OPCODE_MUL,
    OPCODE_DIV,
    OPCODE_CMP_EQ,
    OPCODE_CMP_LT,
    OPCODE_CMP_GT,
    OPCODE_CMP_LEQ,
    OPCODE_CMP_GEQ,
    
    OPCODE_LOAD, // TODO: currently load and store operate on the stack.
               // will need to seperate stack loads from regular loads
    OPCODE_IMM,
    OPCODE_INDEX,
    OPCODE_ADDR,

    OPCODE_STORE,
    OPCODE_COPY,
    OPCODE_RET,

    OPCODE_JMP,
    OPCODE_JMP_EQ,
    OPCODE_JMP_LT,
    OPCODE_JMP_GT,
    OPCODE_JMP_GE,
    OPCODE_JMP_LE,
    OPCODE_JMP_NEQ,
    OPCODE_CALL,
    OPCODE_PARAM,

    OPCODE_COUNT,
} Opcode;

typedef enum {
    ADDR_NONE = 0,
    ADDR_VREG,
    ADDR_REG,
    //ADDR_LABEL,
} AddrType; 

typedef struct {
    AddrType type;
    union {
        int vreg;
        PhysicalReg reg;
    };
} Addr;

typedef struct {
	Addr base;
	Addr index;
	int offset;
	int stride;
	int size;
} MemRef;


typedef enum {
    OPERAND_NONE = 0,
    OPERAND_VREG,
    OPERAND_SLOT,
    OPERAND_REG,
    OPERAND_IMM,
    OPERAND_MEM,
    OPERAND_LABEL, // label id now
    OPERAND_GLOBAL
} OperandType;


typedef struct {
    OperandType type;
    union {
        int vreg;
        PhysicalReg reg;
        int slot;
        int imm;
        int label_id;
        int glbl;
    		MemRef mem;
    };
} Operand;


typedef struct {
    Opcode opcode;
    Operand dest;
    Operand arg1;
    Operand arg2;
} Instr;


typedef struct {
    Instr *items;
    size_t count;
    size_t capacity;
} Bytecode;

typedef struct {
    int size;
    bool sign;
} VregInfo;

typedef struct {
    VregInfo *items;
    size_t count;
    size_t capacity;
} VregInfoTable;

typedef struct {
    int *items; // label to addr inside that bytecode
    size_t count;
    size_t capacity;
} LabelTable;

typedef struct {
    int size;
    bool sign;
    bool address_taken;
} SlotInfo;

typedef struct {
    SlotInfo *items;
    size_t count;
    size_t capacity;
} SlotTable;

typedef struct {
    VregInfoTable vregs;
    LabelTable labels;
    SlotTable slots; // we want slots to stay as vregs but will get demoted to mem under certain rules

    Arena *arena;
    SymbolTable *scope;
    HashTable slot_from_symbol;
    HashTable global_from_symbol;
} EmitContext;


typedef struct {
    Bytecode bytecode;
    VregInfoTable vregs;
    LabelTable labels;
    SlotTable slots;
} Procedure;


typedef struct {
    Procedure *items;
    size_t count;
    size_t capacity;
} Program;


Operand allocate_label(EmitContext *context, Bytecode *bytecode);
Operand allocate_vreg_explicit(EmitContext *context, VregInfo info);
Operand allocate_vreg_type(EmitContext *context, QuectoType *type);
//Operand allocate_slot(EmitContext *context, SymbolData *sym);
Operand slot_for(EmitContext *context, SymbolData *sym);
Operand global_for(EmitContext *context, const char *ident);
Operand gen_instr(Bytecode *bytecode, Opcode opcode, size_t opcount, ...);
void force_label_here(EmitContext *context, Bytecode *bytecode, Operand label);

void emit_program(EmitContext *context, Program *into, AST *program);
void emit_procedure(EmitContext *context, Procedure *into, AST *procedure);
void emit_statement(EmitContext *context, Bytecode *bytecode, AST *statement);

void emit_block(EmitContext *context, Bytecode *bytecode, AST *block);
void emit_if(EmitContext *context, Bytecode *bytecode, AST *_if);
void emit_if_chain(EmitContext *context, Bytecode *bytecode, AST *_if, Operand end_label);
void emit_while(EmitContext *context, Bytecode *bytecode, AST *_while);
void emit_return(EmitContext *context, Bytecode *bytecode, AST *ret);

Operand emit_call(EmitContext *context, Bytecode *bytecode, AST *call, bool has_dest);
Operand emit_lhs(EmitContext *context, Bytecode *bytecode, AST *lhs);
Operand emit_expr(EmitContext *context, Bytecode *bytecode, AST *expr);
void emit_assign(EmitContext *context, Bytecode *bytecode, AST *assign);
void emit_decl(EmitContext *context, Bytecode *bytecode, AST *decl);

void print_procedure(Procedure proc);










#endif
