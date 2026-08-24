#ifndef TRUETYPE_RENDERER_H_
#define TRUETYPE_RENDERER_H_ 1

#include "truetype_parse.h"
#include "../base/mathf.h"

extern void TT_GlyphColorEdges(TT_GLYPHDATA* glyph);

// Signed-distance field.
extern unsigned char* TT_RenderGlyph_SDF(
    TT_FONTINFO* info, float scale, int glyphIndex,
    int padding, unsigned char onEdgeValue, float pixelDistScale,
    int* width, int* height, int* xoff, int* yoff);

extern void TT_RenderGlyph_Scanline(STACKALLOC* alloc,
    unsigned char* dest, int destW, int destH,
    float scale, float padding);

extern void TT_GetGlyphBitmapBoxSubpixel(TT_FONTINFO* info, int glyph,
    float scaleX, float scaleY, float shiftX, float shiftY, int* ix0, int* iy0, int* ix1, int* iy1);

extern int TT_ComputeCrossingsX(float x, float y, vec2* verts, int nverts, TT_GLYPHDATA* const glyph);

#endif /* TRUETYPE_RENDERER_H_ */
