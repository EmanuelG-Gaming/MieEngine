#ifndef VM_ASSEMBLY_H_
#define VM_ASSEMBLY_H_ 1

#include "../base/base_defs.h"
#include "../base/base_string.h"

#include "anm_manager.h"

#include <unordered_map>
#include <string>

/*
   A custom Assembly format for VMs.
*/

#define ASM_TOKEN_MAX_LEN 64
#define ASM_LABEL_COUNT 32

typedef enum ASM_TOKEN_TYPE {
    TOKEN_EOF = 0,

    TOKEN_VAL,
    TOKEN_NUMBER_LIT,
    TOKEN_CHAR_LIT,
    TOKEN_STR_LIT,

    TOKEN_IDENT,

    LABEL_T,
    REGION_T,

    KW_END,
    KW_PUSH,
    KW_POP,
    KW_XCHG,
    KW_ADD,
    KW_SUB,
    KW_MUL,
    KW_DIV,
    KW_MOD,
    KW_AND,
    KW_OR,
    KW_LES,
    KW_LEQ,
    KW_GTR,
    KW_GEQ,
    KW_JMP,
    KW_JZ,
    KW_JNZ,
    KW_CALL,
    KW_RET,
    KW_GOTO,

    KW_PRINT,
} ASM_TOKEN_TYPE;

/*
typedef struct ASM_TOKEN_VALUE {
    union {
        i64 i;
        char c;
        const char* s;
    };
} ASM_TOKEN_VALUE;
*/

typedef struct ASM_TOKEN {
    ASM_TOKEN_TYPE type;

    int textLen;
    char text[ASM_TOKEN_MAX_LEN + 1];
} ASM_TOKEN;

typedef struct ASM_TOKEN_NODE {
    ASM_TOKEN* current;
    //struct ASM_TOKEN_NODE* prev;
    struct ASM_TOKEN_NODE* next;
    int nTokens;
} ASM_TOKEN_NODE;


/*
   Linked lists (chains) for labels and instruction opcodes.

   In a two-pass asssembler, it converts Assembly into machine code by scanning
   the source program twice.
   The first pass builds a table of labels and memory addresses,
   and the second pass uses that table to replace labels with actual addresses
   and generate the final machine code.
*/
typedef struct ASM_TOKEN_CHAIN {
    ASM_TOKEN_NODE* first;
    ASM_TOKEN_NODE* last;
} ASM_TOKEN_CHAIN;


typedef struct ASM_LABEL {
    // Address is ordered by how the label is defined in the source code.
    uintptr_t address;
    // Offset from main address.
    // The main entry point label is the
    // first that gets executed, so the main entry point has the lowest address,
    // while the first defined label has the next address.
    uintptr_t offset;
    STRING8 name;
} ASM_LABEL;

typedef struct ASM_LABEL_TABLE {
    ASM_TOKEN_CHAIN tokenChain;
    ASM_LABEL labels[ASM_LABEL_COUNT];
    int nLabels;
} ASM_LABEL_TABLE;


typedef struct ASM_RESULT {
    ARENA* arena;

    ANM_VM* srcVM;

    // TODO: Find better names for this.
    uintptr_t instrArgCounter;


    INSTR_NODE* instructions;
    LITERAL_TYPES* types;
    int nInstructions;

    //STRING8 cursor;
    int line;

    // Labels.
    //ASM_LABEL* labels;
    //int nLabels;
    ASM_LABEL_TABLE* labelTable;

    ASM_TOKEN_CHAIN opcodeChain;

    b32 valid;
} ASM_RESULT;


extern ASM_RESULT VM_Assemble(ANM_VM* vm, STRING8 assembly, const char* entryPoint);
extern void VM_AssembleTerminate(ASM_RESULT* res);


// Should have been stringToKeywords
/*
static const std::unordered_map<const char*, Opcode> stringToOpcode = {
    // "End" archetype.
    { "endl", ENDL },
    { "endv", ENDV },
    // "Push" archetype.
    { "pushl", PUSHL },
    { "pushv", PUSHV },

    { "pop", POP },
    { "xchg", XCHG },
    { "add", ADD },
    { "sub", SUB },
    { "mul", MUL },
    { "div", DIV },
    { "mod", MOD },
    { "and", AND },
    { "or", OR },
    { "les", LES },
    { "leq", LEQ },
    { "gtr", GTR },
    { "geq", GEQ },
    { "jmp", JMP },
    { "jz", JZ },
    { "jnz", JNZ },
    { "call", CALL },
    // "Return" archetype.
    { "ret", RET },
    { "retl", RETL },
    { "retv", RETV },

    // "Print" archetype.
    { "printu", PRINTU },
    { "printc", PRINTC },
    { "print", PRINT },
};
*/

/*
   Just that we use a stack-based VM, rather than a register-based one.
*/

/*
   The keywords would be turned into opcodes,
   depending on the surrounding context (literals) of the instruction.

   For example, if you have 'print "Test string"', then it will give the opcode PRINT.
   If you have 'print 5', then it will give the opcode PRINTU.
*/

// Test Custom Assembly structure:
/*
main:         ; "Main" label
   push 67    ; Pushes '67' to the stack top.
   print $0   ; Prints the stack element at the relative index "0" (stack top).
   add $0, 3
   print $0   ; Prints '70'

   call "some_other_function"
   end
*/

// And when you want to get some useful data,
// you simply call VM_Assemble(srcString, "main");
// The 'some_other_function' must be registered in a custom dispatch table beforehand.
// It returns an error if the argument for 'call' is an already-existing keyword.

// Semicolons ';' are discouraged (they're the beginning of comments),
// opting instead for newline '\n' as the separation of instructions.

// To not flood the C++-side enum with unecessary opcodes,
// like having ADDL for "add literal",
// the Assembly format will generate micro-operations for that.
// i.e. 'add $0 3', will become 'PUSHV 0; PUSHL 3; ADD',
// Where we push the literal '3' as a temporary variable to the stack,
// and then we add them, pushing the result back into the stack.

// The ADD opcode works by popping the first 2 values from the stack top,
// and then adding them.




static const std::unordered_map<std::string, ASM_TOKEN_TYPE> stringToKeyword = {
    // "End" archetype.
    { "end", KW_END },
    // "Push" archetype.
    { "push", KW_PUSH },

    { "pop", KW_POP },
    { "xchg", KW_XCHG },
    { "add", KW_ADD },
    { "sub", KW_SUB },
    { "mul", KW_MUL },
    { "div", KW_DIV },
    { "mod", KW_MOD },
    { "and", KW_AND },
    { "or", KW_OR },
    { "les", KW_LES },
    { "leq", KW_LEQ },
    { "gtr", KW_GTR },
    { "geq", KW_GEQ },
    { "jmp", KW_JMP },
    { "jz", KW_JZ },
    { "jnz", KW_JNZ },
    { "call", KW_CALL },
    // "Return" archetype.
    { "ret", KW_RET },
    { "goto", KW_GOTO },

    // "Print" archetype.
    { "print", KW_PRINT },
};


#endif /* VM_ASSEMBLY_H_ */
