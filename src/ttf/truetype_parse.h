#ifndef TRUETYPE_PARSE_H_
#define TRUETYPE_PARSE_H_ 1

#include "../base/base_defs.h"
#include "../base/base_string.h"

#include "truetype.h"

#include "../misc/stack.h"



#define TT_READ_BE16(m) (u16) ( \
    ((u16)((u8 *)(m))[0] << 8) | \
    ((u16)((u8 *)(m))[1]))

#define TT_READ_BE32(m) (u32) ( \
    ((u32)((u8 *)(m))[0] << 24) | \
    ((u32)((u8 *)(m))[1] << 16) | \
    ((u32)((u8 *)(m))[2] << 8)  | \
    ((u32)((u8 *)(m))[3]))

#define TT_TAG(s) (TT_READ_BE32(s))

typedef struct TT_FONTTABLE {
    u32 offset;
    u32 length;
} TT_FONTTABLE;

typedef struct TT_GLYFENTRY {
    u32 offset;
    u32 length;
} TT_GLYFENTRY;



typedef struct TT_FONTINFO {
    b8 initialized;

    b8 cmap_sorted;

    i16 loca_format;
    u16 nGlyphs;
    u16 cmap_format;
    u32 cmap_offset;

    u32 maxGlyphPoints;
    u32 maxGlyphContours;

    TT_FONTTABLE head, glyf, hmtx, loca;

    STRING8 data;
} TT_FONTINFO;

extern void TT_FontInit(STRING8 buf, TT_FONTINFO* info);
extern float TT_ScaleForEm(STRING8 buf, TT_FONTINFO* info, float pixelsPerEm);

extern TT_GLYPHDATA TT_GlyphDataFromIndex(TT_FONTINFO* info, u32 glyphIndex);
// Codepoints are used for loading unicode text.
extern TT_GLYPHDATA TT_GlyphDataFromCodepoint(TT_FONTINFO* info, u32 codepoint);


extern u32 TT_GlyphIndex(TT_FONTINFO* info, u32 codepoint);

extern b32 TT_GetValidateTable(STRING8 buf, u32 table_tag, TT_FONTTABLE* table);

extern b32 TT_ValidateLoca(STRING8 buf, TT_FONTINFO* const info);

extern float TT_ScaleForPixelHeight(STRING8 buf, TT_FONTINFO* const info, float height);

extern TT_GLYFENTRY TT_FindGlyfEntry(TT_FONTINFO* info, u32 glyphIndex);

#endif /* TRUETYPE_PARSE_H_ */
