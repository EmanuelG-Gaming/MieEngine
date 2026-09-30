#include "font.h"
#include "../../base/base_log.h"
#include "../../platform/platform.h"

#include <stdio.h>

#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"


static const unsigned char bitmap_glyphs[95*13] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,// space: 32
0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,// ! : 33
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x36, 0x36, 0x36,
0x00, 0x00, 0x00, 0x66, 0x66, 0xff, 0x66, 0x66, 0xff, 0x66, 0x66, 0x00, 0x00,
0x00, 0x00, 0x18, 0x7e, 0xff, 0x1b, 0x1f, 0x7e, 0xf8, 0xd8, 0xff, 0x7e, 0x18,
0x00, 0x00, 0x0e, 0x1b, 0xdb, 0x6e, 0x30, 0x18, 0x0c, 0x76, 0xdb, 0xd8, 0x70,
0x00, 0x00, 0x7f, 0xc6, 0xcf, 0xd8, 0x70, 0x70, 0xd8, 0xcc, 0xcc, 0x6c, 0x38,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x1c, 0x0c, 0x0e,
0x00, 0x00, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0c,
0x00, 0x00, 0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x18, 0x30,
0x00, 0x00, 0x00, 0x00, 0x99, 0x5a, 0x3c, 0xff, 0x3c, 0x5a, 0x99, 0x00, 0x00,
0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0xff, 0xff, 0x18, 0x18, 0x18, 0x00, 0x00,
0x00, 0x00, 0x30, 0x18, 0x1c, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x60, 0x60, 0x30, 0x30, 0x18, 0x18, 0x0c, 0x0c, 0x06, 0x06, 0x03, 0x03,
0x00, 0x00, 0x3c, 0x66, 0xc3, 0xe3, 0xf3, 0xdb, 0xcf, 0xc7, 0xc3, 0x66, 0x3c,
0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78, 0x38, 0x18,
0x00, 0x00, 0xff, 0xc0, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0xe7, 0x7e,
0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0x7e, 0x07, 0x03, 0x03, 0xe7, 0x7e,
0x00, 0x00, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0xff, 0xcc, 0x6c, 0x3c, 0x1c, 0x0c,
0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0, 0xff,
0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc7, 0xfe, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e,
0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x03, 0x03, 0xff,
0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e,
0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x03, 0x7f, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e,
0x00, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x30, 0x18, 0x1c, 0x1c, 0x00, 0x00, 0x1c, 0x1c, 0x00, 0x00, 0x00,
0x00, 0x00, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06,
0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60,
0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x0c, 0x06, 0x03, 0xc3, 0xc3, 0x7e,
0x00, 0x00, 0x3f, 0x60, 0xcf, 0xdb, 0xd3, 0xdd, 0xc3, 0x7e, 0x00, 0x00, 0x00,
0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0x66, 0x3c, 0x18,
0x00, 0x00, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe,
0x00, 0x00, 0x7e, 0xe7, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e,
0x00, 0x00, 0xfc, 0xce, 0xc7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc7, 0xce, 0xfc,
0x00, 0x00, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0xc0, 0xff,
0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0xff,
0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xcf, 0xc0, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e,
0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e,
0x00, 0x00, 0x7c, 0xee, 0xc6, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
0x00, 0x00, 0xc3, 0xc6, 0xcc, 0xd8, 0xf0, 0xe0, 0xf0, 0xd8, 0xcc, 0xc6, 0xc3,
0x00, 0x00, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xdb, 0xff, 0xff, 0xe7, 0xc3,
0x00, 0x00, 0xc7, 0xc7, 0xcf, 0xcf, 0xdf, 0xdb, 0xfb, 0xf3, 0xf3, 0xe3, 0xe3,
0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xe7, 0x7e,
0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe,
0x00, 0x00, 0x3f, 0x6e, 0xdf, 0xdb, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x66, 0x3c,
0x00, 0x00, 0xc3, 0xc6, 0xcc, 0xd8, 0xf0, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe,
0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0x7e, 0xe0, 0xc0, 0xc0, 0xe7, 0x7e,
0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xff,
0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
0x00, 0x00, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
0x00, 0x00, 0xc3, 0xe7, 0xff, 0xff, 0xdb, 0xdb, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
0x00, 0x00, 0xc3, 0x66, 0x66, 0x3c, 0x3c, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3,
0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3,
0x00, 0x00, 0xff, 0xc0, 0xc0, 0x60, 0x30, 0x7e, 0x0c, 0x06, 0x03, 0x03, 0xff,
0x00, 0x00, 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c,
0x00, 0x03, 0x03, 0x06, 0x06, 0x0c, 0x0c, 0x18, 0x18, 0x30, 0x30, 0x60, 0x60,
0x00, 0x00, 0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc3, 0x66, 0x3c, 0x18,
0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x38, 0x30, 0x70,
0x00, 0x00, 0x7f, 0xc3, 0xc3, 0x7f, 0x03, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xfe, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
0x00, 0x00, 0x7e, 0xc3, 0xc0, 0xc0, 0xc0, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x7f, 0xc3, 0xc3, 0xc3, 0xc3, 0x7f, 0x03, 0x03, 0x03, 0x03, 0x03,
0x00, 0x00, 0x7f, 0xc0, 0xc0, 0xfe, 0xc3, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0xfc, 0x30, 0x30, 0x30, 0x33, 0x1e,
0x7e, 0xc3, 0x03, 0x03, 0x7f, 0xc3, 0xc3, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0,
0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00,
0x38, 0x6c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x00, 0x00, 0x0c, 0x00,
0x00, 0x00, 0xc6, 0xcc, 0xf8, 0xf0, 0xd8, 0xcc, 0xc6, 0xc0, 0xc0, 0xc0, 0xc0,
0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78,
0x00, 0x00, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xfe, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xfc, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
0xc0, 0xc0, 0xc0, 0xfe, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0x00, 0x00, 0x00, 0x00,
0x03, 0x03, 0x03, 0x7f, 0xc3, 0xc3, 0xc3, 0xc3, 0x7f, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe0, 0xfe, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xfe, 0x03, 0x03, 0x7e, 0xc0, 0xc0, 0x7f, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x1c, 0x36, 0x30, 0x30, 0x30, 0x30, 0xfc, 0x30, 0x30, 0x30, 0x00,
0x00, 0x00, 0x7e, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3, 0xc3, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xc3, 0xe7, 0xff, 0xdb, 0xc3, 0xc3, 0xc3, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xc3, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0xc3, 0x00, 0x00, 0x00, 0x00,
0xc0, 0x60, 0x60, 0x30, 0x18, 0x3c, 0x66, 0x66, 0xc3, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xff, 0x60, 0x30, 0x18, 0x0c, 0x06, 0xff, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x0f, 0x18, 0x18, 0x18, 0x38, 0xf0, 0x38, 0x18, 0x18, 0x18, 0x0f,
0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
0x00, 0x00, 0xf0, 0x18, 0x18, 0x18, 0x1c, 0x0f, 0x1c, 0x18, 0x18, 0x18, 0xf0,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x8f, 0xf1, 0x60, 0x00, 0x00, 0x00,  // : 126
};

// Could do some more?
#define FONT_GLYPH_COUNT 256

#define ATLAS_DEFAULT_WIDTH 512
#define ATLAS_DEFAULT_HEIGHT 512


#define ColorLerp_RGBA_Array(out, a, b, t) do { \
    (out)[0] = LERP((a)[0], (b)[0], t); \
    (out)[1] = LERP((a)[1], (b)[1], t); \
    (out)[2] = LERP((a)[2], (b)[2], t); \
    (out)[3] = LERP((a)[3], (b)[3], t); \
} while (0)


/*
   Draw state.
*/

extern int FontDrawSetColor(FontDrawState* state, float r, float g, float b, float a)
{
    state->col1[0] = r;
    state->col1[1] = g;
    state->col1[2] = b;
    state->col1[3] = a;
    return 0;
}
extern int FontDrawSetColor2(FontDrawState* state, float r, float g, float b, float a)
{
    state->col2[0] = r;
    state->col2[1] = g;
    state->col2[2] = b;
    state->col2[3] = a;
    return 0;
}
extern int FontDrawSetBorderColor(FontDrawState* state, float r, float g, float b, float a)
{
    state->borderColor1[0] = r;
    state->borderColor1[1] = g;
    state->borderColor1[2] = b;
    state->borderColor1[3] = a;
    return 0;
}
extern int FontDrawSetBorderColor2(FontDrawState* state, float r, float g, float b, float a)
{
    state->borderColor1[0] = r;
    state->borderColor1[1] = g;
    state->borderColor1[2] = b;
    state->borderColor1[3] = a;
    return 0;
}


extern int FontDrawSetMode(FontDrawState* ds, FontColorMode mode)
{
    if (ds->mode != mode) {
        ds->mode = mode;

    }
    return 0;
}
extern int FontDrawSetFlags(FontDrawState* ds, b32 flags)
{
    ds->page->flags |= flags;
    return 0;
}

// Resets to some sane defaults.
extern int FontDrawReset(FontDrawState* ds, FontAtlasPage* page)
{
    _MEMSET(ds, 0, sizeof(*ds));
    ds->page = page;
    ds->borderThickness = 1;

    // A font's "theme".
    FontDrawSetColor(ds, 1.0, 1.0, 1.0, 1);
    FontDrawSetColor2(ds, 1.0, 1.0, 1.0, 1);
    FontDrawSetBorderColor(ds, 0.1, 0.1, 0.1, 1);
    FontDrawSetBorderColor2(ds, 0.1, 0.1, 0.1, 1);
    FontDrawSetMode(ds, FONT_COLOR_UD);
    FontDrawSetFlags(ds, FONT_FLAG_RELATIVE);// | FONT_FLAG_BORDERED);


    return 0;
}

static int CalculateBoundingBox(FontGlyph* glyph,
    FontAtlasPage* page, char ch, int* mX, int* mY, int* MX, int* MY)
{
#define UPPER_BOUND +10000
#define LOWER_BOUND -10000
    int idx = ch - ' ';

    int minX, minY, maxX, maxY;
    minX = minY = UPPER_BOUND;
    maxX = maxY = LOWER_BOUND;

    switch (page->fontType)
    {
        case FONT_KIND_BITMAP:
        {
            for (int y = 0; y < glyph->height; ++y)
            {
                u32 scanlineBits = page->bitmap.data[(glyph->height * idx) + glyph->height - 1 - y];

                for (int x = 0; x < glyph->width; ++x)
                {
                    u32 pixel = scanlineBits & (1 << (glyph->width - 1 - x));

                    if (pixel)
                    {
                        minX = MIN(minX, x);
                        minY = MIN(minY, y);
                        maxX = MAX(maxX, x);
                        maxY = MAX(maxY, y);
                    }
                }
            }
        } break;

        case FONT_KIND_TTF:
        {
            // TODO: Handle TTF fonts.
        } break;

        case FONT_KIND_NONE:
        default: return 0;
    }

    if (mX && minX != UPPER_BOUND) *mX = minX;
    if (mY && minY != UPPER_BOUND) *mY = minY;
    if (MX && maxX != LOWER_BOUND) *MX = maxX;
    if (MY && maxY != LOWER_BOUND) *MY = maxY;

    return 0;
}

// Glyph initialization.
extern int GlyphInit(FontGlyph* glyph, FontAtlasPage* page,
    int originX, int originY, int width, int height, char ch)
{
    if (glyph == NULL)
    {
        // TODO: Print error here?
        return -1;
    }

    _MEMSET(glyph, 0, sizeof(*glyph));
    glyph->x = originX;
    glyph->y = originY;
    glyph->width = width;
    glyph->height = height;
    glyph->ch = ch;

    // We calculate its bounding box.
    CalculateBoundingBox(glyph, page, ch, &glyph->minX, &glyph->minY, &glyph->maxX, &glyph->maxY);

    // We calculate the advance by taking the sum of the left bearing, the glyph width and the right bearing
    int left = 0, right = 0;
    int bottom = 0, top = 0;

    left = glyph->minX;
    right = glyph->width - glyph->maxX;

    bottom = glyph->minY;
    top = glyph->height - glyph->maxY;

    // We then find the texture coordinates.
    glyph->u1 = (glyph->x) / (float) page->image->width;
    glyph->v1 = (glyph->y) / (float) page->image->height;
    glyph->u2 = (glyph->x + glyph->width) / (float) page->image->width;
    glyph->v2 = (glyph->y + glyph->height) / (float) page->image->height;


    int w = glyph->maxX - glyph->minX;
    int h = glyph->maxY - glyph->minY;

    // The advance X and Y are calculated from the baseline bearing.
    glyph->advanceX = left + w + right;
    glyph->advanceY = bottom + h + top;

    glyph->bearY = 0;
    //glyph->advanceX = w;
    //glyph->advanceY = h;
    //glyph->advanceX = right;
    //glyph->advanceY = h;


    return 0;
}

static int EstimateAtlasSize(FontAtlasPage* page, int stoppingWidth, int* atlasWidth, int* atlasHeight)
{
    int advX = 0;
    int advY = 0;
    int maxRowHeight = 0;

    for (int i = page->visible.begin; i < page->visible.end; ++i)
    {
        float x, y;
        FontGlyph glyph;

        x = advX; y = advY;
        GlyphInit(&glyph, page, x, y, page->bitmap.w, page->bitmap.h, i);

        advX += glyph.width;
        maxRowHeight = MAX(maxRowHeight, glyph.height);

        if (advX >= stoppingWidth)
        {
            // Records a row.
            advX = 0;
            advY += maxRowHeight;
            maxRowHeight = glyph.height;
        }
    }
    // Align to next power of two.
    *atlasWidth = ALIGN_UP_POW2(stoppingWidth, 128);
    *atlasHeight = ALIGN_UP_POW2(advY + maxRowHeight, 128);

    //fprintf(stderr, "%s: Atlas width: %d, atals height: %d\n", __func__, *atlasWidth, *atlasHeight);

    return 0;
}



static int RasterBitmap(SoftImage* dest, char ch,
    unsigned char* bitmap, FontGlyph* const glyph)
{
    u32 idx = ch - ' ';

    // Rasterize font.
    unsigned char* g = &bitmap[glyph->height * idx];
    for (i32 y = 0; y < glyph->height; ++y)
    {
        u32 scanline_bits = g[(glyph->height - 1 - y)];

        for (i32 x = 0; x < glyph->width; ++x)
        {
            u32 src = scanline_bits & (1 << (glyph->width - 1 - x));
            unsigned char* pixel = &dest->data[((y) * dest->width + (x)) * 4];

            if (src)
            {
                // Could do some sub-pixel antialiasing here.
                u8 val = ((y / (float) glyph->height) + (x / (float) glyph->width)) * 255;
                //u8 val = 0xff;

                pixel[0] = val;
                pixel[1] = val;
                pixel[2] = val;
                pixel[3] = 0xff;
            }
        }
    }

    return 0;
}


static inline void ApplyColorPixel(FontAtlasPage* page, FontGlyph* const glyph,
    unsigned char* pixel, int x, int y)
{
    float* color = NULL;
    float tmp[4];

    switch (page->ds.mode)
    {
        case FONT_COLOR_1:
            color = page->ds.col1;
            break;
        case FONT_COLOR_2:
            color = page->ds.col2;
            break;

        case FONT_COLOR_CHECKERBOARD:
            if ((x ^ y) & 1) {
                color = page->ds.col1;
            } else {
                color = page->ds.col2;
            }
            break;
        case FONT_COLOR_DIAGONAL:
            if (((x << 1) + y) & 3) {
                color = page->ds.col1;
            } else {
                color = page->ds.col2;
            }

            break;
        case FONT_COLOR_PSEUDORANDOM:
            // Simple hash.
            if ((x * 123 + y * 456) & 1) {
                color = page->ds.col1;
            } else {
                color = page->ds.col2;
            }
            break;

        case FONT_COLOR_LR:
        {
            // Gradient effect (interpolates between colors).
            float factor = (page->flags & FONT_FLAG_RELATIVE) ? glyph->width : (glyph->maxX - glyph->minX);
            ColorLerp_RGBA_Array(tmp, page->ds.col1, page->ds.col2, x / factor);
            color = tmp;
        } break;
        case FONT_COLOR_UD:
        {
            float factor = (page->flags & FONT_FLAG_RELATIVE) ? glyph->height : (glyph->maxY - glyph->minY);
            ColorLerp_RGBA_Array(tmp, page->ds.col1, page->ds.col2, y / factor);
            color = tmp;
        } break;


        default: break;
    }

    if (color == NULL)
    {
        return;
    }

    pixel[0] = color[0] * 255;
    pixel[1] = color[1] * 255;
    pixel[2] = color[2] * 255;
    //pixel[3] = color[3] * 255;
}

static int ColorizeImage(FontAtlasPage* page, FontGlyph* const glyph, SoftImage* dest)
{
    // We colorize each pixel.
    for (int y = 0; y < dest->clip.h; ++y)
    {
        for (int x = 0; x < dest->clip.w; ++x)
        {
            unsigned char* pixel = &dest->data[(y * dest->width + x) * 4];

            if (pixel[3])
            {
                ApplyColorPixel(page, glyph, pixel, x, y);
            }
        }
    }

    return 0;
}

static int TranslateImage(SoftImage* dest, SoftImage* src,
    int dx, int dy)
{
    int startX = MAX(dest->clip.x, dx);
    int startY = MAX(dest->clip.y, dy);
    int endX = MIN(dest->clip.x + dest->clip.w, dx + src->clip.w);
    int endY = MIN(dest->clip.y + dest->clip.h, dy + src->clip.h);

    for (int y = startY; y < endY; ++y)
    {
        for (int x = startX; x < endX; ++x)
        {
            int srcX = x - dx;
            int srcY = y - dy;

            int destIndex = 4 * (y * dest->width + x);
            int srcIndex = 4 * (srcY * src->width + srcX);

            _MEMCPY(&dest->data[destIndex], &src->data[srcIndex], 4);
        }
    }

    return 0;
}


static int FlipImageY(SoftImage* dest)
{
    const size_t bpp = 4;
    size_t full_rowsize = bpp * dest->width;
    size_t clip_rowsize = bpp * dest->clip.w;
    unsigned char* tmp = (unsigned char *) _MALLOC(clip_rowsize);
    if (tmp == NULL)
    {
        return -1;
    }

    int startY = dest->clip.y;
    int height = dest->clip.h;
    int cxb = dest->clip.x * bpp;

    for (int i = 0; i < height >> 1; ++i)
    {
        int y1 = startY + i;
        int y2 = startY + height - 1 - i;

        int idx1 = y1 * full_rowsize + cxb;
        int idx2 = y2 * full_rowsize + cxb;

        // Swap.
        _MEMCPY(tmp, dest->data + idx1, clip_rowsize);
        _MEMCPY(dest->data + idx1, dest->data + idx2, clip_rowsize);
        _MEMCPY(dest->data + idx2, tmp, clip_rowsize);
    }

    _FREE(tmp);

    return 0;
}

// Used for things like text borders.
static int ExpandImage(FontAtlasPage* page, FontGlyph* const glyph, int borderThickness,
    SoftImage* dest, SoftImage* src
) {
    // Use border color.
    unsigned char r = page->ds.borderColor1[0] * 255.0f;
    unsigned char g = page->ds.borderColor1[1] * 255.0f;
    unsigned char b = page->ds.borderColor1[2] * 255.0f;
    unsigned char a = page->ds.borderColor1[3] * 255.0f;

    for (int y = 0; y < src->clip.h + borderThickness * 2; ++y)
    {
        for (int x = 0; x < src->clip.w + borderThickness*2 ; ++x)
        {
            unsigned char* srcPixel = &src->data[(y * src->width + x) * 4];
            if (srcPixel[3])
            {

                // Iterate over the glyph (kinda like a convolution).
                for (int dx = -1; dx <= 1; ++dx)
                {
                    for (int dy = -1; dy <= 1; ++dy)
                    {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < dest->clip.w + borderThickness * 2 && ny >= 0 && ny < dest->clip.h + borderThickness * 2)
                        {
                            unsigned char* p = &dest->data[(ny * dest->width + nx) * 4];
                            if (!p[3])
                            {
                                p[0] = r;
                                p[1] = g;
                                p[2] = b;
                                p[3] = a;
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}

static int UpdatePageStep(FontAtlasPage* page, FontGlyph* glyph)
{
    // Update + add glyph.
    page->x1 = MIN(page->x1, glyph->x);
    page->y1 = MIN(page->y1, glyph->y);
    page->x2 = MAX(page->x2, glyph->x + glyph->width);
    page->y2 = MAX(page->y2, glyph->y + glyph->height);


    return 0;
}

/*
static int AddGlyphToPage(FontAtlasPage* page, FontGlyph* glyph)
{
    // Why would the font have no glyph list?

    if (page->glyphs == NULL)
    {
        // Lazy initialization.
        page->glyphCapacity = REGION_DEFAULT_CAPACITY;
        page->glyphCount = 0;
        page->glyphs = (FONTGLYPH *) _MALLOC(sizeof(FONTGLYPH) * page->glyphCapacity);
    }

    if (page->glyphCount >= page->glyphCapacity)
    {
        page->glyphCapacity = page->glyphCapacity + (page->glyphCapacity >> 1);
        page->glyphs = (FONTGLYPH *) _REALLOC(page->glyphs, sizeof(FONTGLYPH) * page->glyphCapacity);
    }

    // Also update the size of the region as we go with it.
    UpdatePageStep(page, glyph);

    // For text borders, we wanna make it fit.
    int differenceW = 0;
    int differenceH = 0;
    if (page->flags & FONT_FLAG_BORDERED)
    {
        differenceW = page->ds.borderThickness * 2;
        differenceH = page->ds.borderThickness * 2;
    }

    int clipW = glyph->width + differenceW;
    int clipH = glyph->height + differenceH;
    int capW = glyph->width * 2;
    int capH = glyph->height * 2;

    // We create 3 temporary images.
    SoftImage *tmp1 = SoftImageInitClip(clipW, clipH, capW, capH),
              *tmp2 = SoftImageInitClip(clipW, clipH, capW, capH),
              *tmp3 = SoftImageInitClip(clipW, clipH, capW, capH);

    // First pass: Copy the font from the bitmap/sprite into the temporary image.
    if (page->fontType == FONT_KIND_BITMAP)
    {
        RasterBitmap(tmp1, glyph->ch, page->rom.bitmapData.data, glyph);
    }
    else
    {
        int idx = glyph->ch - ' ';
        IMAGEDATA* image = &page->rom.images[idx];

        int nChannels = GetNumChannels(image->format);
        SoftImageBlitProOptimized(tmp1->data, tmp1->clip.x, tmp1->clip.y, tmp1->clip.w, tmp1->clip.h, tmp1->width, tmp1->height,
            image->data, 0, 0, image->w, image->h, image->w, image->h, nChannels);
    }

    ColorizeImage(page, glyph, tmp1);


    if (page->flags & FONT_FLAG_BORDERED)
    {
        int newW = glyph->width + differenceW;
        int newH = glyph->height + differenceH;

        // Dest, src.
        TranslateImage(tmp2, tmp1, page->ds.borderThickness, page->ds.borderThickness);
        ExpandImage(page, glyph, page->ds.borderThickness, tmp3, tmp2);

        SoftImageBlit(tmp3, tmp2);

        glyph->width = newW;
        glyph->height = newH;
    }

    SoftImageBlitOptimized((page->flags & FONT_FLAG_BORDERED) ? tmp3 : tmp1,
        glyph->x, glyph->y, page->data, page->w, page->h, 4);

    SoftImageTerminate(tmp1);
    SoftImageTerminate(tmp2);
    SoftImageTerminate(tmp3);


    page->glyphs[page->glyphCount++] = *glyph;

    return 0;
}
*/

static void AddBitmapGlyphToImage(GFX_font* font, FontAtlasPage* page, FontGlyph glyph)
{
    // Add glyph to list.
    //FontGlyph* added = ArenaPushStruct(font->glyphs.glyphArena, FontGlyph);

    // For text borders, we wanna make it fit.
    int differenceW = 0;
    int differenceH = 0;
    if (page->flags & FONT_FLAG_BORDERED)
    {
        differenceW = page->ds.borderThickness * 2;
        differenceH = page->ds.borderThickness * 2;
    }

    int clipW = glyph.width + differenceW;
    int clipH = glyph.height + differenceH;
    int capW = glyph.width * 2;
    int capH = glyph.height * 2;

    // We create 3 temporary images.
    //ARENA_TEMP maybeTemp = ArenaTempBegin(font->imageArena);
    SoftImage* tmp1 = SoftImageInitClip(font->imageArena, clipW, clipH, capW, capH, 4);

    if (page->fontType == FONT_KIND_BITMAP)
    {
        //LogErrorEmitF("YOOOOO");
        RasterBitmap(tmp1, glyph.ch, page->bitmap.data, &glyph);
    }

    ColorizeImage(page, &glyph, tmp1);

    SoftImageBlitOptimized(tmp1, glyph.x, glyph.y, page->image->data, page->width, page->height, 4);

    //ArenaTempEnd(maybeTemp);

    //UpdatePageStep(page, &glyph);

    // Push glyph into arena.

    font->glyphs.glyphs[font->glyphs.count++] = glyph;
    //fprintf(stderr, "%s: glyph count: %d\n", __func__, font->glyphs.count);
}

extern FontAtlasPage* FontPageInit(GFX_font* font, int pageIndex)
{
    // Create large "canvas".
    FontAtlasPage* page = &font->atlas.pages[pageIndex];
    page->atlas = &font->atlas;
    page->width = ATLAS_DEFAULT_WIDTH;
    page->height = ATLAS_DEFAULT_HEIGHT;
    page->active = TRUE;

    page->texture = NULL;
    page->textureSlot = 0;

    page->advanceX = 0;
    page->advanceY = 0;
    page->maxRowHeight = 0;
    page->flags = FONT_FLAG_NONE;

    // Load draw state.
    FontDrawReset(&page->ds, page);

    // Load texture.
    //size_t bytecount = page->image.
    //page->image = ArenaPushArrayZero(font->arena, u8, bytecount);

    page->image = SoftImageInitOrigin(font->imageArena, page->width, page->height, 4);

    if (pageIndex >= font->atlas.nPages)
    {
        font->atlas.nPages = pageIndex + 1;
    }

    return page;
}

extern FontAtlasPage* FontLoadBitmap(GFX_font* font, int pageIndex,
    unsigned char* bitmap, int charPixelWidth, int charPixelHeight)
{
    FontAtlasPage* page = FontPageInit(font, pageIndex);

    // Initialize bitmap data.
    {
        page->bitmap.data = bitmap;
        page->bitmap.w = charPixelWidth;
        page->bitmap.h = charPixelHeight;
    }

    // Monospaced font.
    page->flags |= FONT_FLAG_MONOSPACE;
    page->fontType = FONT_KIND_BITMAP;
    page->scale = 1.0f;

    return page;
}

extern FontAtlasPage* FontLoadBitmapDefault(GFX_font* font, int pageIndex)
{
    return FontLoadBitmap(font, pageIndex, (unsigned char *) bitmap_glyphs, 8, 13);
}

extern FontAtlasPage* FontLoadTTF(GFX_font* font, int pageIndex, const char* filePath)
{
    // TTF data.
    /*
    stbtt_fontinfo* fontinfo = ArenaPushStruct(font->mainArena, stbtt_fontinfo);
    String8 fontFileBuffer = Platform_fileRead(font->mainArena, STR8_LIT(filePath));
    if (!stbtt_InitFont(fontinfo, fontFileBuffer.data, 0))
    {
        LogInfoEmitF("%s: Failed to init TTF font at path: %s", __func__, filePath);
        return NULL;
    }
    */

    // TODO: This is probably gonna leak some memory.

    //stbtt_fontinfo* fontinfo = ArenaPushStruct(font->mainArena, stbtt_fontinfo);
    u8* ffbData;
    u32 ffbLength;

    Platform_fileRead(font->mainArena, STR8_LIT(filePath), &ffbData, &ffbLength);

    fprintf(stderr, "%s: Font file buffer: %p %u\n", __func__, ffbData, ffbLength);

    stbtt_bakedchar* cdata = ArenaPushArrayZero(font->mainArena, stbtt_bakedchar, 128);


    FontAtlasPage* page = FontPageInit(font, pageIndex);
    /*
    page->ttf.user = fontinfo;
    page->ttf.fontFileBuffer = fontFileBuffer;
    */

    page->flags |= 0;
    page->fontType = FONT_KIND_TTF;
    page->scale = 1.0f;

    // Copy some data.
    unsigned char* redData = ArenaPushArrayZero(font->mainArena, unsigned char, page->width * page->height);
    stbtt_BakeFontBitmap(ffbData, 0, 32.0f, redData, page->width, page->height, 32, 95, cdata);

    // And then fill it up.
    for (int i = 0; i < page->width * page->height; ++i)
    {
        unsigned char* srcPixel = redData + i;
        unsigned char* dstPixel = (unsigned char *) page->image->data + (i * 4);

        dstPixel[0] = srcPixel[0];
        dstPixel[1] = srcPixel[0];
        dstPixel[2] = srcPixel[0];
        dstPixel[3] = srcPixel[0];
    }

    // TODO: And then do the glyphs.
    for (int i = 0; i <= 95; ++i)
    {
        stbtt_bakedchar d = cdata[i];

        FontGlyph glyph = { 0 };
        glyph.ch = ' ' + i;
        glyph.u1 = d.x0 / (float) page->image->width;
        glyph.u2 = d.x1 / (float) page->image->width;
        glyph.v1 = d.y0 / (float) page->image->height;
        glyph.v2 = d.y1 / (float) page->image->height;

        glyph.x = d.x0;
        glyph.y = d.y0;
        glyph.advanceX = d.xadvance;

        glyph.width = d.x1 - d.x0;
        glyph.height = d.y1 - d.y0;
        glyph.bearY = d.yoff;

        font->glyphs.glyphs[font->glyphs.count++] = glyph;
    }

    return page;
}


extern FontAtlasPage* FontPageAdd_ascii(GFX_font* font, int pageIndex)
{
    FontAtlasPage* page = &font->atlas.pages[pageIndex];

    {
        page->visible = ASCII_VISIBLE_CHARSET;
    }

    if (page->fontType == FONT_KIND_BITMAP)
    {

        // Populate region with ASCII charset.
        for (int i = page->visible.begin; i < page->visible.end; ++i)
        {
            FontGlyph glyph;
            GlyphInit(&glyph, page, page->advanceX, page->advanceY, page->bitmap.w, page->bitmap.h, i);
            // Does both rasterization...
            AddBitmapGlyphToImage(font, page, glyph);

            // ...and packing.
            page->advanceX += glyph.width + 1;
            page->maxRowHeight = MAX(page->maxRowHeight, glyph.height + 1);

            if (page->advanceX + 2 >= page->width)
            {
                page->advanceX = 1;
                page->advanceY += page->maxRowHeight + 1;
                page->maxRowHeight = glyph.height + 1;
            }
        }
    }
    else if (page->fontType == FONT_KIND_TTF)
    {
        //stbtt_pack_context packContext;
        //stbtt_PackBegin(&packContext, page->image->data, page->width, page->height, page->width, 1, NULL);

        // Pack ASCII characters (32-126).
        //stbtt_PackFontRange(&packContext, page->ttf.fontFileBuffer.data, 0, 48, page->visible.begin, page->visible.end, page->image->data);

        //stbtt_PackEnd(&packContext);

        /*
        if (!stbtt_InitFont(fontinfo, fontFileBuffer.data, 0))
        {
            LogInfoEmitF("%s: Failed to init TTF font at path: %s", __func__, filePath);
            return NULL;
        }
        */


    }
    else
    {
        LogErrorEmitF("%s: Failed to load text atlas. The font type is not bitmap or TTF!", __func__);
        return NULL;
    }

    //font->atlas.nPages ++;

    return page;
}




//page->rom.bitmapData.format = IMAGE_FORMAT_R;


extern GFX_texture* FontUploadTexture(GFX_font* font, int pageIndex)
{
    FontAtlasPage* page = &font->atlas.pages[pageIndex];
    unsigned int flags = TEXTURE_ALPHA;
    //flags ^= flags;
    page->texture = LoadImmutableTextureFromPixels(font->mainArena, page->width, page->height, page->image->data, flags);

    // We remove the texture from the image.
    ArenaClear(font->imageArena);

    return page->texture;
}

extern GFX_font* FontInit(ARENA* arena)
{
    GFX_font* font = ArenaPushStruct(arena, GFX_font);
    font->mainArena = arena;
    font->imageArena = ArenaInit(MB(8), KB(8), ARENA_FLAG_GROWABLE);


    //LogErrorEmitF("%s: Arena current: %p\n", __func__, arena->current);

    // Init font page atlas.
    font->atlas.nPages = 0;

    // Init the glyph list.
    font->glyphs.glyphArena = ArenaInit(MB(8), KB(64), ARENA_FLAG_GROWABLE);
    font->glyphs.capacity = FONT_GLYPH_COUNT;
    font->glyphs.count = 0;
    font->glyphs.glyphs = ArenaPushArrayZero(font->glyphs.glyphArena, FontGlyph, font->glyphs.capacity);

    //LogErrorEmitF("%s: Glyph array: %p\n", __func__,font->glyphs.glyphs);

    // Initialize image.
    //FontPageInit(font, 0);

    return font;
}

extern void FontTerminate(GFX_font* font)
{
    // Terminate the pages.

    //font->arena = NULL;
    ArenaTerminate(font->glyphs.glyphArena);
    ArenaTerminate(font->imageArena);
}


static const TextDrawStyle defaultStyle = CLITERAL(TextDrawStyle) {
    /*
    // Col1, col2, border color.
    COLOR_DEF(255, 255, 255, 255),
    COLOR_DEF(255, 255, 255, 255),
    COLOR_DEF(0, 0, 0, 255),

    // Shadow.
    COLOR_DEF(50, 50, 50, 255),
    -1.0f,
    -1.0f,

    // Underline.
    2.0f,
    0.1f,

    // Shearing (italic).
    0.5f,
    1.0f,
    */
};

extern int DrawTextGetSize(GFX_font* font, const char* text,
    i32 fontHeight, float spacingX, float spacingY, float scale,
    float* w, float* h)
{
    if (!strlen(text))
    {
        return -1;
    }

    float wdth = 0;
    float hght = 0;

    float currentX = 0;
    float currentY = 0;

    FontAtlasPage* page = &font->atlas.pages[FONT_PAGE_REGULAR];

    FontGlyph* glyph;
    float maxHeight;
    int idx;

    idx = *text - ' ';
    glyph = &font->glyphs.glyphs[idx];

    currentX = 0;
    currentY = 0;
    maxHeight = glyph->height;

    float lastValidHeight = 0;

    const char* ptr = text;
    for (; *ptr;)
    {
        if (*ptr == '\n')
        {
            // Newline character.
            // What if you had 2 newline characters together,
            // like for a space between paragraphs?
            float h = (maxHeight > 0) ? maxHeight : lastValidHeight;
            //currentY -= (h + spacingY) * scale;
            hght += (h + spacingY);
            currentX = 0;

            maxHeight = 0;
            ptr++;

            continue;
        }


        idx = *ptr - ' ';
        glyph = &font->glyphs.glyphs[idx];

        float width = glyph->width;
        float height = glyph->height;

        currentX += (glyph->advanceX + spacingX);
        wdth = MAX(wdth, currentX);

        maxHeight = MAX(maxHeight, height);
        lastValidHeight = maxHeight;

        // Get minimum and maximum widths.

        ptr++;
    }
    wdth *= scale;
    hght *= scale;

    if (w) *w = wdth;
    if (h) *h = hght;

    return 0;
}

extern int DrawTextPro(GFX_font* font, const char* text, float ox, float oy,
    int fontHeight, float spacingX, float spacingY, float scale,
    int flags, const TextDrawStyle* options)
{
    if (options == NULL)
    {
        return DrawTextPro(font, text, ox, oy, fontHeight, spacingX, spacingY, scale,
            flags, &defaultStyle);
    }

    if (!strlen(text))
    {
        return 0;
    }

    FontAtlasPage* page = &font->atlas.pages[FONT_PAGE_REGULAR];

    FontGlyph* glyph;
    float currentX, currentY;
    float maxHeight;
    int idx;

    // TODO: Add some text memory boundaries verification.
    idx = *text - ' ';
    glyph = &font->glyphs.glyphs[idx];

    currentX = ox;
    currentY = oy;
    maxHeight = glyph->height;

    DrawMatIdentity();
    DrawTexture(page->textureSlot, page->texture);

    // Draw state.
    //DrawColorMode(COLOR_UD);
    //DrawColor(1, 1, 1, 1);
    //DrawColor2(0, 0, 0, 1);

    float lastValidHeight = 0;

    int tx = drawState.tex[page->textureSlot].tex->w;
    int ty = drawState.tex[page->textureSlot].tex->h;

    const char* ptr = text;
    for (; *ptr;)
    {
        if (*ptr == '\n')
        {
            // Newline character.
            // What if you had 2 newline characters together,
            // like for a space between paragraphs?
            float h = (maxHeight > 0) ? maxHeight : lastValidHeight;
            currentY -= (h + spacingY) * scale;
            currentX = ox;

            maxHeight = 0;
            ptr++;

            continue;
        }


        idx = *ptr - ' ';
        glyph = &font->glyphs.glyphs[idx];

        float width = glyph->width;
        float height = glyph->height;
        float dwidth = (glyph->maxX - glyph->minX);
        float dheight = (glyph->maxY - glyph->minY);
        float bearingY = glyph->bearY;

        DrawSrcRect(glyph->x, glyph->y, width, height);

        // Add slight offset.

        DrawMatIdentity();
        DrawMatTranslate3D(currentX + width*scale*0.5f, currentY + ((-height*0.5f)-bearingY)*scale, 0);
        DrawRect(width * scale, height * scale);

        currentX += (glyph->advanceX + spacingX) * scale;
        maxHeight = MAX(maxHeight, height);
        lastValidHeight = maxHeight;

        ptr++;
    }
    // Restore previous state.
    DrawSrcRect(0, 0, tx, ty);
    DrawMatIdentity();

    //DrawMatTranslate3D(0, 0, 0);

    return 0;

}
/*
   Example API:
   GFX_font* font = FontInit(initArena);
   FontAtlasPage* page = FontPageInit(font, 0);
   FontLoadBitmapDefault(font, 0);

   // Add ASCII loading.
   FontBlit_ascii();

   GFX_texture* tex = FontUploadTexture(font, 0);
*/

