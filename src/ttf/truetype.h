#ifndef TRUETYPE_H_
#define TRUETYPE_H_ 1

#include "../base/base_defs.h"

// Credits to MagicalBat.

typedef enum TT_POINT_FLAG_ENUM {
    TT_POINT_FLAG_NONE = 0,

    TT_POINT_FLAG_LINE = (1 << 0),
    TT_POINT_FLAG_CONTOUR_END = (1 << 1),
    TT_POINT_FLAG_CONTOUR_OFFSET = (1 << 2),

    TT_POINT_FLAG_GENERATED = (1 << 3),

    // Only used for MSDF rendering.
    TT_POINT_FLAG_RED = (1 << 4),
    TT_POINT_FLAG_GREEN = (1 << 5),
    TT_POINT_FLAG_BLUE = (2 << 6), // TODO: or is it 1 << 6?
} TT_POINT_FLAG_ENUM;

typedef u8 TT_POINTFLAG;

typedef struct vec2i16 {
    i16 x, y;
} vec2i16;

typedef struct TT_GLYPHDATA {
    i16 minX;
    i16 minY;
    i16 maxX;
    i16 maxY;

    u32 nConturs;
    u32 nSegments;
    u32 nPoints;

    TT_POINTFLAG* flags;
    vec2i16* points;
} TT_GLYPHDATA;


#endif /* TRUETYPE_H_ */
