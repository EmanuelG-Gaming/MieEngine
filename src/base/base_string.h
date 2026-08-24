#ifndef BASE_STRING_H_
#define BASE_STRING_H_ 1

#include "base_defs.h"


static inline size_t CstrLen(const char* str)
{
    const char* ptr = str;
    for (; *ptr; ++ptr) {}
    return (size_t) (ptr - str);
}

typedef struct STRING8 {
    u64 len;
    u8* data;
} STRING8;
#define STR8_LIT(str) CLITERAL(STRING8) { CstrLen(str), (u8 *) (str) }
#define STR8_FMT(str) (int) (str).len, (const char *) (str).data


extern int Str8Equal(STRING8 a, STRING8 b);
extern STRING8 Str8Substr(STRING8 base, int a, int b);

#endif /* BASE_STRING_H_ */
