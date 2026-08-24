#include "anm_manager.h"
#include "vm_assembly.h"

#include <cstdint>
#include <string.h>


#define DISPATCH_CAPACITY 64
#define TEXT_SEGMENT_CAPACITY 2048

#define SWAP_REF(a, b) do { \
    DECLTYPE_CAST((b)+0) tmp = (b); \
    *(a) = *(b); \
    *(b) = *(tmp); \
} while(0)


static ANM_MANAGER anm = { 0 };



// NOTE: No bounds checking.

static inline void StackIncr(ANM_Stack* stack)
{
    ++stack->sp;
}
static inline void StackDecr(ANM_Stack* stack)
{
    --stack->sp;
}

// stack = struct { uintptr_t* sp; }
static inline void StackPush(ANM_Stack* stack, uintptr_t value)
{
    *(stack->sp) = value;
    stack->sp++;
}

static inline uintptr_t StackPop(ANM_Stack* stack)
{
    stack->sp--;
    return *(stack->sp);
}

static inline uintptr_t* StackRef(ANM_Stack* stack, int idx)
{
    return (uintptr_t *) (stack->sp - idx);
}
static inline uintptr_t* StackTop(ANM_Stack* stack)
{
    return StackRef(stack, 0);
}



static uintptr_t InstrDispatchDirect(INSTR_NODE* pc, ANM_VM* vm)
{
    //fprintf(stderr, "%s: Global dispatchTable:%p\n", __func__, anm.dispatchTable);
    //fprintf(stderr, "%s: op=%s (%d) dispatchTable ptr:%p\n", __func__, OpcodeToStr(pc->op), pc->op, anm.dispatchTable[pc->op]);
    return anm.dispatchTable[pc->op](pc, vm);
}
static uintptr_t InstrDispatch(INSTR_NODE* pc, ANM_VM* vm)
{
    return InstrDispatchDirect(++pc, vm);
}

/*
   Putting 'inline' in a function for a V-table is risky,
   because the compiler can copy and paste the function.
*/

#define DISPATCH_IMPL

static DISPATCH_IMPL uintptr_t exec_dummy(INSTR_NODE* pc, ANM_VM* vm)
{
    return InstrDispatchDirect(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_endl(INSTR_NODE* pc, ANM_VM* vm)
{
    return pc->arg;
}
static DISPATCH_IMPL uintptr_t exec_endv(INSTR_NODE* pc, ANM_VM* vm)
{
    return StackPop(&vm->stack);
}
static DISPATCH_IMPL uintptr_t exec_pushl(INSTR_NODE* pc, ANM_VM* vm)
{
    // Push literal.
    StackPush(&vm->stack, pc->arg);
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_pushv(INSTR_NODE* pc, ANM_VM* vm)
{
    // Push value from literal.
    StackPush(&vm->stack, *StackRef(&vm->stack, pc->arg));
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_pop(INSTR_NODE* pc, ANM_VM* vm)
{
    for (int i = 0; i < pc->arg; ++i)
    {
        StackPop(&vm->stack);
    }
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_xchg(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t* a = StackTop(&vm->stack);
    uintptr_t* b = StackRef(&vm->stack, pc->arg);

    SWAP_REF(a, b);
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_add(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs + lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_sub(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs - lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_mul(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs * lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_imul(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs * lhs;
    return InstrDispatch(pc, vm);
}

static DISPATCH_IMPL uintptr_t exec_div(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs / lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_idiv(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs / lhs;
    return InstrDispatch(pc, vm);
}

static DISPATCH_IMPL uintptr_t exec_mod(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs % lhs;
    return InstrDispatch(pc, vm);
}

/*
   Bitwise and logical operators.
*/

static DISPATCH_IMPL uintptr_t exec_and(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs & lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_or(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs | lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_xor(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs ^ lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_shl(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs << lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_shr(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs >> lhs;
    return InstrDispatch(pc, vm);
}
/*
static inline uintptr_t exec_sar(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs >> lhs;
    return InstrDispatch(pc, vm);
}
*/

static DISPATCH_IMPL inline uintptr_t exec_neg(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t operand = StackPop(&vm->stack);
    *StackTop(&vm->stack) = ~operand;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_not(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t operand = StackPop(&vm->stack);
    *StackTop(&vm->stack) = !operand;
    return InstrDispatch(pc, vm);
}

static DISPATCH_IMPL uintptr_t exec_equ(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs == lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_neq(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs != lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_les(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs < lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_leq(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs <= lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_gtr(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs > lhs;
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_geq(INSTR_NODE* pc, ANM_VM* vm)
{
    uintptr_t rhs = StackPop(&vm->stack);
    uintptr_t lhs = StackPop(&vm->stack);
    *StackTop(&vm->stack) = rhs >= lhs;
    return InstrDispatch(pc, vm);
}


static DISPATCH_IMPL uintptr_t exec_jmp(INSTR_NODE* pc, ANM_VM* vm)
{
    return InstrDispatchDirect(pc + pc->arg, vm);
}
static DISPATCH_IMPL uintptr_t exec_jz(INSTR_NODE* pc, ANM_VM* vm)
{
    size_t offset = StackPop(&vm->stack) ? 1 : pc->arg;
    return InstrDispatchDirect(pc + offset, vm);
}
static DISPATCH_IMPL uintptr_t exec_jnz(INSTR_NODE* pc, ANM_VM* vm)
{
    size_t offset = StackPop(&vm->stack) ? pc->arg : 1;
    return InstrDispatchDirect(pc + offset, vm);
}
static DISPATCH_IMPL uintptr_t exec_call(INSTR_NODE* pc, ANM_VM* vm)
{
    // Push return address.
    StackPush(&vm->stack, (uintptr_t) pc + 1);
    return InstrDispatchDirect(pc + pc->arg, vm);
}
static DISPATCH_IMPL uintptr_t exec_ret(INSTR_NODE* pc, ANM_VM* vm)
{
    pc = (INSTR_NODE *) StackPop(&vm->stack);
    return InstrDispatchDirect(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_retl(INSTR_NODE* pc, ANM_VM* vm)
{
    // TODO: Unary op.
    return InstrDispatchDirect(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_retv(INSTR_NODE* pc, ANM_VM* vm)
{
    return InstrDispatchDirect(pc, vm);
}

/* Printing/debugging functions. */
static DISPATCH_IMPL uintptr_t exec_printu(INSTR_NODE* pc, ANM_VM* vm)
{
    printf("[VM]: %u\n", (unsigned int) pc->arg);
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_printc(INSTR_NODE* pc, ANM_VM* vm)
{
    printf("[VM]: %c\n", (char) pc->arg);
    return InstrDispatch(pc, vm);
}
static DISPATCH_IMPL uintptr_t exec_print(INSTR_NODE* pc, ANM_VM* vm)
{
    printf("[VM]: %s\n", (const char *) pc->arg);
    return InstrDispatch(pc, vm);
}

static DISPATCH_IMPL uintptr_t exec_none(INSTR_NODE* pc, ANM_VM* vm)
{
    return InstrDispatch(pc, vm);
}



ALIGN_DECL(64) static const ANM_FUNCTION_SIGNATURE defaultDispatchTable[] = {
    exec_dummy,
    exec_endl,
    exec_endv,
    exec_pushl,
    exec_pushv,
exec_pop,
    exec_xchg,
    exec_add, exec_sub, exec_mul, exec_div, exec_idiv, exec_mod,
    exec_and, exec_or, exec_xor, exec_shl, exec_shr,
    exec_neg, exec_not,
    exec_equ, exec_neq, exec_les, exec_leq, exec_gtr, exec_geq,

    exec_jmp, exec_jz, exec_jnz,
    exec_call,
    exec_ret, // Return.
    exec_retl, // Return literal.
    exec_retv, // Return and push stack pos.

    exec_printu,
    exec_printc,
    exec_print,

    exec_none,
};


extern ANM_VM* VM_Init(ARENA* arena)
{
    ANM_VM* vm = (ANM_VM *) ArenaPush(arena, sizeof(ANM_VM), 0);
    if (vm == NULL)
    {
        fprintf(stderr, "%s: Failed to init VM!\n", __func__);
        return NULL;
    }

    vm->current = vm;
    //vm->next = NULL;

    //vm->ip = 0;

    //vm->sp = -1;

    //vm->bytecode = NULL;
    //vm->codeSize = -1;

    vm->flags = 0;

    return vm;
}

extern void VM_Halt(ANM_VM* vm)
{
    vm->flags |= VM_STATUS_HALTED;
}


/*
   ANM Manager.
*/

extern int ANM_Init(void)
{
    _MEMSET(&anm, 0, sizeof(anm));

    anm.arena = ArenaInit(MB(4), KB(16), ARENA_FLAG_GROWABLE);
    anm.vmCount = 0;

    //fprintf(stderr, "%s: ANM info: arena:%p sp:%zu\n", __func__, anm.arena, anm.arena->pos);

    size_t bytecount = sizeof(defaultDispatchTable);
    // We push the default dispatch table.
    anm.dispatchTable = (ANM_FUNCTION_SIGNATURE *) ArenaPush(anm.arena, bytecount, 0);
    _MEMCPY(anm.dispatchTable, defaultDispatchTable, bytecount);

    anm.nDispatches = MAX_OPCODE;

    //fprintf(stderr, "%s: ANM info: arena:%p sp:%zu\n", __func__, anm.arena, anm.arena->pos);
    //fprintf(stderr, "%s: Dispatch table: %p\n", __func__, anm.dispatchTable);

    /*
    for (int i = 0; i < STATIC_ARR_LEN(defaultDispatchTable) + 1; ++i)
    {
        fprintf(stderr, "[%s] = %p\n", OpcodeToStr((Opcode) i), anm.dispatchTable[i]);
    }
    */


    return 0;
}

extern void ANM_Terminate(void)
{
    ArenaTerminate(anm.arena);
}


static INSTR_NODE defaultInstr[] = {
    {
        PRINTC,
        (uintptr_t) 'c',
    },
    {
        ENDL,
        67, // Haha funny number
    },
};

static const char* defaultAssembly =
    ".DATA:\n"
    "function:\n"
    "among_us:\n"
    "ret\n"
    "\n"
    "main:\n"
    "    push 31\n"
    "    pop\n"
    "    push 3\n"
    "    push 5\n"
    "    push $67\n"
    "    pop 2 ; Pops 2 elements\n"
    "    print $0\n"
    "    print 'a'\n"
    //"    xchg $1, $2\n"
    "    call \"some_external_function\"\n"
    "    call function\n" // Identifiers.
    "    \n"
    "loop:\n"
    "    print 5\n"
    "    jmp loop\n"
    "endLoop:"
    "    end\n"
    //"funny:\n"
    ;

extern ANM_VM* ANM_CreateVM(void)
{
    // Memory structure of the VM:
    // [VM struct][text segment][stack].
    ANM_VM* vm = VM_Init(anm.arena);

    // Put the text segment first.
    // TODO: Maybe do this when we declare a text segment in the
    // custom Assembly format.
    vm->textSegment = (ANM_Segment *) ArenaPush(anm.arena, TEXT_SEGMENT_CAPACITY, 0);
    vm->textSegment->begin = (uintptr_t) vm->textSegment;
    vm->textSegment->offset = 0;

    // Fill in some data (we push the VM and the stack buffer in the same arena).
    uintptr_t* stackBuffer = (uintptr_t *) ArenaPush(anm.arena, ANM_STACK_SIZE, 0);
    vm->stack.sp = stackBuffer;


    // DEFAULT VALUES!
    // We set some instructions (for default).
    VM_SetInstructions(vm, defaultInstr, STATIC_ARR_LEN(defaultInstr));


    // Print.
    ASM_RESULT r = VM_Assemble(vm, STR8_LIT(defaultAssembly), "main");
    VM_AssembleTerminate(&r);

    // Do some testing.
    LITERAL_TYPES literals[50] = {
        INSTR_LIT_CHAR,
        INSTR_LIT_INT,
    };

    PrintInstructions(defaultInstr, literals, STATIC_ARR_LEN(defaultInstr));

    return vm;
}

extern int VM_SetInstructions(ANM_VM* vm, INSTR_NODE* instr, int nInstr)
{
    vm->pc = instr;

    return 0;
}

extern int PrintInstructions(INSTR_NODE* instructions, LITERAL_TYPES* types, int nInstr)
{
    puts("\nINSTR DUMP:");
    for (int i = 0; i < nInstr; ++i)
    {
        INSTR_NODE* instr = &instructions[i];
        LITERAL_TYPES literal = types[i];

        // Opcode, literal.
        printf("%s: ", OpcodeToStr(instr->op));
        switch (literal)
        {
            case INSTR_LIT_INT: printf("%d", (int) instr->arg); break;
            case INSTR_LIT_CHAR: printf("%c", (int) instr->arg); break;
            case INSTR_LIT_POINTER: printf("0x%02x", (unsigned int) instr->arg); break;
            case INSTR_LIT_STR: printf("%s", (const char *) instr->arg); break;
            default: printf("UNKNOWN LITERAL"); break;
        }
        puts("");
    }
    puts("\n");

    return 0;
}



/*
extern int ANM_Push(ANM_VM* vm, u8* bytecode, int codeSize)
{
    // Uses previously-allocated vm.

    //vm->bytecode = bytecode;
    //vm->codeSize = codeSize;

    // We append the VM.
    //anm.vms[anm.count++] = vm;

    return 0;
}
*/

/*
extern int ANM_Tick(void)
{
    for (int i = 0; i < anm.vmCount; ++i)
    {
        ANM_VM* vm = anm.vms[i];
        if (vm->flags & VM_STATUS_HALTED)
        {
            continue;
        }

        // Fetch, decode, execute cycle.
        //vm->ip += 4;
    }

    return 0;
}

*/

extern uintptr_t VM_Draw(ANM_VM* vm)
{

    if (vm->flags & VM_STATUS_HALTED)
    {
        return 0;
    }

    //Opcode op;
    /*
    sie_t i = 0;
    do
    {
    } while (op != ENDL && op != ENDV);
    */
    uintptr_t ret = InstrDispatchDirect(vm->pc, vm);
    //vm->pc += 1;

    //fprintf(stderr, "%s: ret: %d\n", __func__, (int) ret);

    return ret;
}
