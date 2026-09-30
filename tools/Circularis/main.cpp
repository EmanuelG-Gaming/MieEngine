#define ARENA_IMPLEMENTATION
#include "../../ext/arena.h"
#include "../../ext/arena.cpp"

//#include "circularis_asm.h"
#include "compile_config.h"

#include "runtime/vm.h"
#include "runtime/vm.cpp"


static inline int fti(float x)
{
    int res;
    memcpy(&res, &x, sizeof(x));
    return res;

    //return *((int *) &x);
}



#define CUSTOM_FN_REGION 1

static uintptr_t exec_vec2Print(VM* vm)
{
    uintptr_t x = ccl::PopVal(vm);
    uintptr_t y = ccl::PopVal(vm);
    fprintf(stderr, "vec2(%zu, %zu)\n", x, y);

    return ccl::InstrDispatch(vm);
}

static uintptr_t exec_vec3Print(VM* vm)
{
    uintptr_t x = ccl::PopVal(vm);
    uintptr_t y = ccl::PopVal(vm);
    //uintptr_t z = ccl::PopVal(vm);
    //fprintf(stderr, "vec3(%zu, %zu, %zu)\n", x, y, z);

    return ccl::InstrDispatch(vm);
}

ALIGN_DECL(64) static VM_functionSignature* dispTable[] = {
    exec_vec2Print//, exec_vec3Print,
};


int VM_test(void)
{
    VM_rawInstr instr[] = {
        // Opcode, offset, time, varMask, args.
        /*
        { PUSHL, 1, 0, 0, { 3 } },
        { PUSHL, 1, 0, 0, { 5 } },
        { ADD, 1, 0, 0, { 0 } },
        { PRINTU, 1, 0, 0, { 0 } },
        */

        { EXEC, 1, 0, 0b0011, { CUSTOM_FN_REGION, 0, 5, 3 } },
        { EXEC, 1, 0, 0b0011, { CUSTOM_FN_REGION, 0, 6, 7 } },
        //{ EXEC, 1, 0, 0b0111, { CUSTOM_FN_REGION, 1, 6, 7, 3 } },

        { PUSHL, 1, 0, 0, { 67423757 } },
        { POP, 1, 0, 0, { 1 } },
        { PRINTU, 1, 0, 0, { 0 } },

        { ENDL, 0, 0, 0, { 68 } },
    };

    ARENA* arena = ArenaInit(MB(4), KB(4), ARENA_FLAG_GROWABLE);

    ccl::VM* vm = ccl::VM_init(arena);
    ccl::VM_SetInstructions(vm, instr, 1);
    ccl::VM_SetFunctionsFFI(vm, CUSTOM_FN_REGION, STATIC_ARR_LEN(dispTable), dispTable);

    for (int i = 0; i < 10; ++i)
    {
        //printf("i: %d\n", i);

        ccl::VM_Tick(vm);
        /*
        if (ret != 0)
        {
            fprintf(stderr, "%s: coroutine stopped!\n", __func__);
            ccl::VM_halt(vm);

            break;
        }
        */
    }

    ArenaTerminate(arena);

    return 0;
}

int main(int argc, const char* argv[])
{
    VM_test();

    return 0;
}
