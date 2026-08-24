#ifndef ANM_MANAGER_H_
#define ANM_MANAGER_H_ 1

#include "../base/base_defs.h"

#include "../misc/arena.h"


#define ANM_VM_HEADER_SIZE 128

#define ANM_VM_CAPACITY 32

// 2048 bytes.
#define ANM_STACK_SIZE 2048

#define ANM_LAYER_CAPACITY 8

typedef enum Opcode {
    DUMMY,
    ENDL, // End parsing, return literal value
    ENDV, // End parsing, return stack top,
    PUSHL, // Push literal
    PUSHV, // Push value from stack
    POP,
    XCHG,
    ADD, SUB, MUL, DIV, IDIV, MOD, //IMOD,
    AND, OR, XOR, SHL, SHR,// SAR,
    NEG, NOT,
    EQU, NEQ, LES, LEQ, GTR, GEQ, //BEL, ABV,
    //AEQ,
    JMP, JZ, JNZ,
    CALL,
    RET, // Return
    RETL, // Return literal
    RETV, // Return and push stack top

    PRINTU, PRINTC, PRINT,

    MAX_OPCODE,
} Opcode;

typedef enum LITERAL_TYPES {
    INSTR_LIT_INT,
    INSTR_LIT_CHAR,
    INSTR_LIT_POINTER,
    INSTR_LIT_STR,
} LITERAL_TYPES;

typedef enum VM_FLAGS {
    VM_STATUS_HALTED = (1 << 0),
} VM_FLAGS;

typedef enum DRAW_VM_OBJ {
    DRAW_VM_RECT,
    DRAW_VM_ELLIPSE,
} DRAW_VM_OBJ;



typedef struct ANM_VM ANM_VM;
typedef struct INSTR_NODE INSTR_NODE;

typedef uintptr_t (*ANM_FUNCTION_SIGNATURE)(INSTR_NODE*, ANM_VM*);

typedef struct INSTR_NODE {
    Opcode op;

    uintptr_t arg;
} INSTR_NODE;


typedef struct ANM_Segment {
    // Real address.
    uintptr_t begin;
    uintptr_t offset;
} ANM_Segment;


/*
   Animation engine.
*/

typedef struct ANM_VM ANM_VM;

typedef struct ANM_LAYER {
    //int order;
    struct ANM_VM* prev;
    struct ANM_VM* next;
} ANM_LAYER;

typedef struct ANM_Stack {
    uintptr_t* sp;
} ANM_Stack;

typedef struct ANM_VM {
    struct ANM_VM* current;

    ANM_Segment* textSegment;

    //ANM_LAYER layers[ANM_LAYER_CAPACITY];
    //int nLayers;

    ANM_FUNCTION_SIGNATURE** dispatchTable;
    int nFunctions;

    //u8* bytecode;
    //uintptr_t stack[ANM_STACK_SIZE];

    INSTR_NODE* pc;

    //int codeSize;
    //int ip;
    //int sp;
    ANM_Stack stack;

    u32 flags;
} ANM_VM;

/* A 'draw VM' will encapsulate state about virtual shapes. */
typedef struct DRAW_VM {
    DRAW_VM_OBJ geom;

    struct DRAW_VM* next;
    int nLayers;


    float color1[4];
    float color2[4];
} DRAW_VM;

//STATIC_GETSIZE(ANM_VM);

typedef struct ANM_MANAGER {
    ARENA* arena;

    ANM_FUNCTION_SIGNATURE* dispatchTable;
    int nDispatches;

    ANM_VM* vms[ANM_VM_CAPACITY];
    int vmCount;
} ANM_MANAGER;





extern int ANM_Init(void);
extern void ANM_Terminate(void);

// Push bytecode into VM.
//extern int ANM_Push(ANM_VM* vm, u8* bytecode, int codeSize);

extern ANM_VM* ANM_CreateVM(void);
extern int ANM_Tick(void);

extern ANM_VM* VM_Init(ARENA* arena);
extern void VM_Terminate(ANM_VM* vm);

extern int VM_SetInstructions(ANM_VM* vm, INSTR_NODE* instr, int nInstr);

extern uintptr_t VM_Draw(ANM_VM* vm);


// Utility.
extern int PrintInstructions(INSTR_NODE* instructions, LITERAL_TYPES* types, int nInstr);


static const char* OpcodeToStr(Opcode op)
{
    switch (op)
    {
        case DUMMY: return "dummy";
        case ENDL: return "endl";
        case ENDV: return "endv";
        case PUSHL: return "pushl";
        case PUSHV: return "pushv";
        case POP: return "pop";
        case XCHG: return "xchg";
        case ADD: return "add";
        case SUB: return "sub";
        case MUL: return "mul";
        case DIV: return "div";
        case IDIV: return "idiv";
        case MOD: return "mod";
        //case IMOD: return "imod";
        case AND: return "and";
        case OR: return "or";
        case LES: return "les";
        case LEQ: return "leq";
        case GTR: return "gtr";
        case GEQ: return "geq";
        case JMP: return "jmp";
        case JZ: return "jz";
        case JNZ: return "jnz";
        case CALL: return "call";
        case RET: return "ret";
        case RETL: return "retl";
        case RETV: return "retv";

        case PRINTU: return "printu";
        case PRINTC: return "printc";
        case PRINT: return "print";

        case MAX_OPCODE: return "MAX OPCODE CONTROL VAR";
        default: return "UNKNOWN";
    }
}

#endif /* ANM_MANAGER_H_ */
