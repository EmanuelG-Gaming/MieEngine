#ifndef ANM_VARS_H_
#define ANM_VARS_H_ 1

#include "../base/base_defs.h"
#include "anm_vm.h"

/*
   Some macros for easier bytecode development.
*/

/*
   Opcodes.
*/

/*
#define ANM_NOP 0
#define ANM_DESTROY 1
#define ANM_FREEZE 2

#define ANM_SPRITE 3
#define ANM_JMP 4
#define ANM_JMP_DEC 5

#define ANM_ISET 6
#define ANM_FSET 7

#define ANM_IADD 8
#define ANM_FADD 9
#define ANM_ISUB 10
#define ANM_FSUB 11
#define ANM_IMUL 12
#define ANM_FMUL 13
#define ANM_IDIV 14
#define ANM_FDIV 15
#define ANM_IMOD 16
#define ANM_FMOD 17

#define ANM_ISETADD 18
#define ANM_FSETADD 19
#define ANM_ISETSUB 20
#define ANM_FSETSUB 21
#define ANM_ISETMUL 22
#define ANM_FSETMUL 23
#define ANM_ISETDIV 24
#define ANM_FSETDIV 25
#define ANM_ISETMOD 26
#define ANM_FSETMOD 27

#define ANM_IJE 28
#define ANM_FJE 29
#define ANM_IJNE 30
#define ANM_FJNE 31
#define ANM_IJL 32
#define ANM_FJL 33
#define ANM_IJLE 34
#define ANM_FJLE 35
#define ANM_IJG 36
#define ANM_FJG 37
#define ANM_IJGE 38
#define ANM_FJGE 39

#define ANM_ISETRAND 40
#define ANM_FSETRAND 41

#define ANM_FSIN 42
#define ANM_FCOS 43
#define ANM_FTAN 44
#define ANM_FACOS 45
#define ANM_FATAN 46
#define ANM_WRAPANGLE 47

#define ANM_POS 48
#define ANM_ROTATE 49
#define ANM_SCALE 50
#define ANM_ALPHA 51
#define ANM_COLOR 52

#define ANM_ANGLE_VEL 53
#define ANM_SCALE_GROWTH 54

#define ANM_ALPHATIME_LINEAR 55
#define ANM_POSTIME 56

#define ANM_COLORTIME1 57
#define ANM_ALPHATIME1 58

#define ANM_COLORTIME2 59
#define ANM_ALPHATIME2 60

#define ANM_FLIPX 61
#define ANM_FLIPY 62

#define ANM_STOP 63

#define ANM_INTERRUPT_LABEL 64
#define ANM_UNKNOWN_65 65

#define ANM_BLENDMODE 66
#define ANM_TYPE 67

#define ANM_LAYER 68
#define ANM_STOPHIDE 69
*/


/*
   Registers.
*/

/*
#define ANM_I1 10000
#define ANM_I2 10001
#define ANM_I3 10002
#define ANM_I4 10003

#define ANM_F1 10004
#define ANM_F2 10005
#define ANM_F3 10006
#define ANM_F4 10007

#define ANM_IRAND 10010

#define ANM_POSX 10013
#define ANM_POSY 10014
#define ANM_POSZ 10015

#define ANM_FRAND 10022
*/

/*
   Instruction size.
*/
#define ANM_INSTRSIZE 48
#define ANM_OFFSET(n) ((n) * ANM_INSTRSIZE)
#define ANM_NEXT ANM_INSTRSIZE

static const char* ANM_getInstrName(uint16_t opcode)
{
    switch (opcode)
    {
        case ANM_NOP: return "nop";
        case ANM_DESTROY: return "destroy";
        case ANM_FREEZE: return "freeze";
        case ANM_SPRITE: return "sprite";
        case ANM_JMP: return "jmp";
        case ANM_JMP_DEC: return "jmpDec";
        case ANM_ISET: return "iset";
        case ANM_FSET: return "fset";
        case ANM_IADD: return "iadd";
        case ANM_FADD: return "fadd";
        case ANM_ISUB: return "isub";
        case ANM_FSUB: return "fsub";
        case ANM_IMUL: return "imul";
        case ANM_FMUL: return "fmul";
        case ANM_IDIV: return "idiv";
        case ANM_FDIV: return "fdiv";
        case ANM_IMOD: return "imod";
        case ANM_FMOD: return "fmod";

        case ANM_ISETADD: return "iSetAdd";
        case ANM_FSETADD: return "fSetAdd";
        case ANM_ISETSUB: return "iSetSub";
        case ANM_FSETSUB: return "fSetSub";
        case ANM_FSETMUL: return "fSetMul";
        case ANM_ISETDIV: return "iSetDiv";
        case ANM_FSETDIV: return "fSetDiv";
        case ANM_ISETMOD: return "iSetMod";
        case ANM_FSETMOD: return "fSetMod";

        default: return "UNKNOWN OPCODE";
    }
}

#endif /* ANM_VARS_H_ */
