#ifndef STRING_UTILS_H_
#define STRING_UTILS_H_ 1

#include "types.h"

static inline u32 CstrLen(const char* str)
{
    const char* ptr = str;
    for (; *ptr; ++ptr) {}
    return (u32) (ptr - str);
}

typedef struct String8 {
    u8* data;
    u32 len;
} String8;

#define STR8_LIT(cstr) { (u8 *) (data), CstrLen(data) }
#define STR8_FMT(str) (int) (str).len, (const char *) (str).data


#endif /* STRING_UTILS_H_ */
