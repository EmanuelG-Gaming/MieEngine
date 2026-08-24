/*
   Font software renderer.

   Features:
   - Composable effects (gradients, noise, etc).
   - Alpha-blended text borders.
   - Texture atlas pages.
*/

#ifndef FONT_H_
#define FONT_H_ 1

#include "../base/base_defs.h"
#include "texture.h"
#include "color.h"
#include "../misc/stack.h"


//#define MAX_FONT_REGIONS 8
#define MAX_FONT_PAGES 8

#define FONT_FLAG_NONE 0
#define FONT_FLAG_BOLD 1
#define FONT_FLAG_UNDERLINE 2
#define FONT_FLAG_ITALIC 4
#define FONT_FLAG_STRIKETHROUGH 8
#define FONT_FLAG_SHADOW 16
#define FONT_FLAG_MONOSPACE 32
#define FONT_FLAG_BORDERED 64
#define FONT_FLAG_RELATIVE 128

typedef enum FONTKIND {
    FONT_KIND_NONE = 0,
    FONT_KIND_BITMAP,
    FONT_KIND_TTF,
    FONT_KIND_OTF,
} FONTKIND;

typedef enum FONT_COLORMODE {
    FONT_COLOR_1,
    FONT_COLOR_2,
    FONT_COLOR_UD,
    FONT_COLOR_LR,
    FONT_COLOR_CHECKERBOARD,
    FONT_COLOR_DIAGONAL,
    FONT_COLOR_PSEUDORANDOM,
} FONT_COLORMODE;

typedef enum FONT_PAGE {
    FONT_PAGE_REGULAR,
    FONT_PAGE_BORDER,
} FONT_PAGE;

typedef enum IMAGE_BLENDMODE {
    IMAGE_BLEND_NONE,

    IMAGE_BLEND_ALPHA,
    IMAGE_BLEND_ADD,
    IMAGE_BLEND_SUB,
    IMAGE_BLEND_MUL,
    IMAGE_BLEND_DIV,

    // Inverse of multiply.
    IMAGE_BLEND_SCREEN,
    IMAGE_BLEND_REPLACE,
    IMAGE_BLEND_OVERLAY,

    IMAGE_BLEND_COUNT,
} IMAGE_BLENDMODE;


/*
   A software image.
*/

typedef enum IMAGEFORMAT {
    IMAGE_FORMAT_R,
    IMAGE_FORMAT_RGBA,
} IMAGEFORMAT;


typedef struct CPUIMAGE {
    unsigned char* data;
    int width, height;
    // Internal rectangle, for more
    // constrained areas.
    struct {
        int x, y;
        int w, h;
    } clip;

    IMAGE_BLENDMODE blend;
} CPUIMAGE;

extern CPUIMAGE* SoftImageInit(int clipX, int clipY, int clipW, int clipH, int capW, int capH);
extern CPUIMAGE* SoftImageInitClip(int clipW, int clipH, int capW, int capH);
extern CPUIMAGE* SoftImageInitOrigin(int capW, int capH);

extern void SoftImageTerminate(CPUIMAGE* image);

extern int SoftImageResizeUpper(CPUIMAGE* image, int capW, int capH);

extern void SoftImageSetClip(CPUIMAGE* image, int x, int y, int w, int h);
extern void SoftImageSetBlend(CPUIMAGE* image, IMAGE_BLENDMODE mode);

extern int SoftImageLock(CPUIMAGE* image);
extern int SoftImageUnlock(CPUIMAGE* image);


/*
   Optimized versions (no blending).
*/

extern void SoftImageBlitProOptimized(
         unsigned char* dest, int cx1, int cy1, int cw1, int ch1, int destW, int destH,
    unsigned char* const src, int cx2, int cy2, int cw2, int ch2, int srcW, int srcH, int nChannels);

extern void SoftImageBlitOptimized(CPUIMAGE* dest, CPUIMAGE* src, int nChannels);
extern void SoftImageBlitOptimized(CPUIMAGE* dest,
    unsigned char* const src, int srcX, int srcY, int srcW, int srcH, int nChannels);
extern void SoftImageBlitOptimized(CPUIMAGE* const src, int srcX, int srcY,
    unsigned char* dest, int destW, int destH, int nChannels);

/*
   Normal versions (with blending).
*/

extern void SoftImageBlitPro(
    IMAGE_BLENDMODE generalBlend,
         unsigned char* dest, int cx1, int cy1, int cw1, int ch1, int destW, int destH,
    unsigned char* const src, int cx2, int cy2, int cw2, int ch2, int srcW, int srcH, int nChannels);

extern void SoftImageBlit(CPUIMAGE* dest, CPUIMAGE* src);





/*
   Mostly for character encodings.
*/

// The most widely-known charset is
// ASCII, which begins at index 0 and ends at index 127.
typedef struct CHARSET {
    int begin, end;
} CHARSET;
#define ASCII_CHARSET CLITERAL(CHARSET) { 0, 127 }
#define ASCII_VISIBLE_CHARSET CLITERAL(CHARSET) { 32, 127 }



typedef struct FONTGLYPH {
    // Minimal AABB of glyph.
    int minX, minY;
    int maxX, maxY;

    // Location in the font atlas (texture coordinates).
    float u1, v1;
    float u2, v2;

    // Basic metrics for the blit image.
    // x and y are the relative position in relation
    // to the baseline where the glyph should be drawn.
    int x, y;
    int width, height;

    int advanceX;
    int advanceY;

    char ch;
} FONTGLYPH;

struct FONTATLAS;
struct FONTATLASPAGE;

typedef struct FONT_DRAWSTATE {
    struct FONTATLASPAGE* page;

    float col1[4];
    float col2[4];
    float borderColor[4];
    FONT_COLORMODE mode;

    int borderThickness;

    //b32 flags;
} FONT_DRAWSTATE;


typedef struct IMAGEDATA {
    unsigned char* data;
    int w, h;
    IMAGEFORMAT format;
} IMAGEDATA;

typedef struct FONT_ROMDATA {
    IMAGEDATA bitmapData;
    // IMAGE DATA (RGBA).
    IMAGEDATA* images;

    size_t glyphCount;
    size_t glyphCapacity;
} FONT_ROMDATA;

typedef struct FONTATLASPAGE {
    struct FONTATLAS* atlas;

    CHARSET fullSet;
    CHARSET visible;

    // Dynamic array of glyph elements.
    size_t glyphCount;
    size_t glyphCapacity;
    FONTGLYPH* glyphs;

    FONT_DRAWSTATE ds;
    FONTKIND fontType;

    // Texture.
    TEXTURE* texture;

    // The image itself.
    unsigned char* data;
    int w, h;

    FONT_ROMDATA rom;

    // Minimum bounding box.
    int x1, y1;
    int x2, y2;

    // Current advance x and y.
    int advanceX, advanceY;
    int maxRowHeight;

    b32 flags;
} FONTATLASPAGE;



typedef struct FONTATLAS {
    FONTATLASPAGE pages[MAX_FONT_PAGES];
    size_t nPages;
    // size_t regionCapacity;

    // BITMAP DATA.
    /*
    unsigned char* bitmapData;
    // The number of pixels in a font.
    int bitmapCharWidth;
    int bitmapCharHeight;
    */
} FONTATLAS;

typedef struct FONT {
    FONTATLAS atlas;
    STACKALLOC stack; // Internal stack.

    //TEXTURE* texture;
    int textureSlot;
} FONT;

// TODO: Could probably use a state machine.

typedef struct TEXTDRAWSTYLE {
    COLOR tintColor1;
    COLOR tintColor2;
    COLOR borderColor;

    COLOR shadowColor;
    float shadowX, shadowY;

    float underlinePadding;
    float underlineThickness;

    float shearX, shearY;
} TEXTDRAWSTYLE;

extern int GlyphInit(FONTGLYPH* glyph, FONTATLASPAGE* region, int originX, int originY, int width, int height, char ch);

/*
   Draw state.
*/

extern int FontDrawReset(FONT_DRAWSTATE* ds, FONTATLASPAGE* page);

extern int FontDrawSetColor(FONT* font, float r, float g, float b, float a);
extern int FontDrawSetColor2(FONT* font, float r, float g, float b, float a);
extern int FontDrawSetBorderColor(FONT* font, float r, float g, float b, float a);

extern int FontDrawSetMode(FONT* font, FONT_COLORMODE mode);
extern int FontDrawSetFlags(FONT* font, b32 flags);

// charPixelWidth must be a multiple of 8.
extern FONT* LoadFontBitmap(unsigned char* bitmap, int charPixelWidth, int charPixelHeight);
extern FONT* LoadFontTTF(const char* filePath);

extern FONT* LoadDefaultFont(void);

extern FONTATLASPAGE* FontInitPage(FONT* font, int pageIndex, FONTKIND type);

extern TEXTURE* UploadFont(FONT* font, int pageIndex);

//extern int FontInitBitmap(FONT* font, int bitwidth);
extern void FontTerminate(FONT* font);
extern void FontPageReleaseTexture(FONT* font, int id);

//extern float DrawFontGetTextWidth(FONT *font, const char *text, i32 fontHeight, float scale);
extern int DrawTextPro(FONT* font, const char* text,
    float ox, float oy, i32 fontHeight, float spacingX, float spacingY, float scale,
    int flags = 0, const TEXTDRAWSTYLE* options = NULL);

#endif /* FONT_H_ */
