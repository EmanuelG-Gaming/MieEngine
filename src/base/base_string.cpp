#include "base_string.h"
#include "../mem/arena.h"

#include <string.h>
#include <stdarg.h>

/*
   For vsnprintf :skul:
   TODO: Use custom string formatting.
*/
#include <stdio.h>


extern int Str8_equals(String8 a, String8 b)
{
    if (a.len != b.len)
    {
        return FALSE;
    }

    for (size_t i = 0; i < a.len; ++i)
    {
        if (a.data[i] != b.data[i])
        {
            return FALSE;
        }
    }

    return TRUE;
}



extern String8 Str8_substr(String8 src, u64 start, u64 end)
{
    end = MIN(src.len, end);
    start = MIN(end, start);

    return CLITERAL(String8) {
        end - start,
        src.data + start,
    };
}

extern u64 Str8_findFirst(String8 src, u8 ch)
{
    u64 i = 0;

    for (; i < src.len; ++i)
    {
        if (src.data[i] == ch) break;
    }

    return i;
}

extern String8 Str8_fromCstr(u8* cstr)
{
    u8* start = cstr;

    for (; *cstr; ++cstr) {};

    return CLITERAL(String8) {
        (u64) (cstr - start),
        start,
    };
}



extern void Str8_memcpy(String8* dest, const String8* src, u64 offset)
{
    if (offset > dest->len)
    {
        return;
    }

    u64 size = MIN(src->len, dest->len - offset);
    memcpy(dest->data + offset, src->data, size);
}

extern String8 Str8_copy(ARENA* arena, String8 src)
{
    String8 out;
    out.data = ArenaPushArrayNZ(arena, u8, src.len);
    out.len = src.len;

    memcpy(out.data, src.data, src.len);

    return out;
}

// Push format variadic argument.
extern String8 Str8_pushfv(ARENA* arena, const char* fmt, va_list args)
{
    String8 out = { 0 };

    va_list args2;
    va_copy(args2, args);

    int size = vsnprintf(NULL, 0, fmt, args);

    if (size > 0)
    {
        ARENA_TEMP maybeTemp = ArenaTempBegin(arena);

        out.len = (u64) size;
        out.data = ArenaPushArrayNZ(maybeTemp.arena, u8, out.len + 1);

        size = vsnprintf((char *) out.data, out.len + 1, fmt, args2);

        if (size <= 0)
        {
            out = CLITERAL(String8) { 0 };
            ArenaTempEnd(maybeTemp);
        }
    }

    va_end(args2);

    return out;
}

extern String8 Str8_pushf(ARENA* arena, const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    String8 out = Str8_pushfv(arena, fmt, args);

    va_end(args);

    return out;
}


/*
   String encoding utilities.
*/


/*
   String decoding utils.
*/

extern StringDecode StrDecodeUTF8(String8 str, u64 offset)
{
    static u8 lengths[] = {
        1, 1, 1, 1, // 000xx
        1, 1, 1, 1,
        1, 1, 1, 1,
        1, 1, 1, 1,
        0, 0, 0, 0, // 100xx
        0, 0, 0, 0,
        2, 2, 2, 2, // 110xx
        3, 3,       // 1110x
        4,          // 11110
        0,          // 11111
    };

    static u8 firstByteMask[] = { 0, 0x7f, 0x1f, 0x0f, 0x07 };
    static u8 finalShift[] = { 0, 18, 12, 6, 0 };

    StringDecode out;
    // Replacement character.
    out.codepoint = 0xfffd;
    out.size = 0;

    if (offset >= str.len)
    {
        return out;
    }

    u32 firstByte  = str.data[offset];
    u32 len = lengths[firstByte >> 3];

    if (len <= 0 || offset + len > str.len)
    {
        return out;
    }

    u32 codepoint = (firstByte & firstByteMask[len]) << 18;
    switch (len)
    {
        case 4: codepoint |= (((u32) str.data[offset+3] & 0x3f));
        case 3: codepoint |= (((u32) str.data[offset+2] & 0x3f) << 6);
        case 2: codepoint |= (((u32) str.data[offset+1] & 0x3f) << 12);
        default: break;
    }

    codepoint >>= finalShift[len];

    out.codepoint = codepoint;
    out.size = len;
 
    return out;
}

extern u32 StrEncodeUTF16(u16* dst, u32 codePoint)
{
    u32 size = 0;

    if (codePoint < 0x010000)
    {
        dst[0] = (u16) codePoint;
        size = 1;
    }
    else
    {
        // Surrogate pairs.
        dst[0] = (u16)((codePoint >> 10) + 0xd800);
        dst[1] = (u16)((codePoint & 0x3ff) + 0xdc00);
        size = 2;
    }

    return size;
}

/*
   Windows uses UTF-16 for wchar_t.
*/

extern String16 Str16FromStr8(ARENA* arena, String8 base, b32 nullTerminate)
{
    u64 maxSize = base.len + (nullTerminate ? 1 : 0);

    u16* out = (u16 *) ArenaPush(arena, sizeof(u16) * maxSize, 0);

    u64 outSize = 0;
    u64 offset = 0;

    while (offset < base.len)
    {
        StringDecode decode = StrDecodeUTF8(base, offset);

        offset += decode.size;

        outSize += StrEncodeUTF16(out + outSize, decode.codepoint);
    }

    u64 requiredChars = outSize + (nullTerminate ? 1 : 0);
    u64 unusedChars = maxSize = requiredChars;
    ArenaPop(arena, unusedChars * sizeof(u16));

    return CLITERAL(String16) {
        outSize,
        out,
    };
}
