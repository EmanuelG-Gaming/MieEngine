#ifndef VM_INSTR_H_
#define VM_INSTR_H_ 1

#include "../base/types.h"

typedef struct VM_rawInstr {
    int16_t opcode; // 0x00
    int16_t offsetToNextInstr; // 0x02
    short time; // 0x04
    uint16_t varMask; // 0x06
    int args[10];
} VM_rawInstr;



#endif /* VM_INSTR_H_ */
