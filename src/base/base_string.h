#ifndef BASE_STRING_H_
#define BASE_STRING_H_ 1

#include "base_defs.h"
//#include "../mem/arena.h"



static inline size_t CstrLen(const char* str)
{
    const char* ptr = str;
    for (; *ptr; ++ptr) {}
    return (size_t) (ptr - str);
}

static inline size_t WstrLen(const wchar_t* str)
{
    const wchar_t* ptr = str;
    for (; *ptr; ++ptr) {}
    return (size_t) (ptr - str);
}




typedef struct String8 {
    size_t len;
    u8* data;
} String8;

#define STR8_LIT(cstr) CLITERAL(String8) { CstrLen(cstr), (u8 *) cstr }
// TODO: Hmmm...
#define STR8_CONST_LIT(cstr) { sizeof(cstr)-1, (u8 *) cstr }

#define STR8_FMT(str) (int) (str).len, (const char *) (str).data


typedef struct String16 {
    size_t len;
    u16* data;
} String16;


typedef struct StringDecode {
    u32 size;
    u32 codepoint;
} StringDecode;

extern int Str8_equals(String8 a, String8 b);

extern String8 Str8_substr(String8 src, u64 start, u64 end);
extern u64 Str8_findFirst(String8 src, u8 ch);
extern String8 Str8_fromCstr(u8* cstr);


extern void Str8_memcpy(String8* dest, const String8* src, u64 offset);
extern String8 Str8_copy(void* arena, String8 src);

extern String8 Str8_pushfv(void* arena, const char* fmt, va_list args);
extern String8 Str8_pushf(void* arena, const char* fmt, ...);

/*
   String encoding utilities.
*/

extern StringDecode StrDecodeUTF8(String8 str, u64 offset);
extern u32 StrEncodeUTF16(u16 *dst, u32 codePoint);
extern String16 Str16FromStr8(void* arena, String8 base, b32 nullTerminate);

#endif /* BASE_STRING_H_ */
