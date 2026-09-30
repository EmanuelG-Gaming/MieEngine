#ifndef FONT_H_
#define FONT_H_ 1

#include "../../base/base_defs.h"
#include "../../base/base_string.h"

#include "../draw.h"
#include "../image/image.h"

#define FONT_PAGE_REGULAR 0

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

typedef enum FontKind {
    FONT_KIND_NONE = 0,
    FONT_KIND_BITMAP,
    FONT_KIND_TTF,
    //FONT_KIND_OTF,
} FontKind;

typedef enum FontColorMode {
    FONT_COLOR_1,
    FONT_COLOR_2,
    FONT_COLOR_UD,
    FONT_COLOR_LR,
    FONT_COLOR_CHECKERBOARD,
    FONT_COLOR_DIAGONAL,
    FONT_COLOR_PSEUDORANDOM,
} FontColorMode;

/*
   Mostly for character encodings.
*/

typedef struct Charset {
    int begin, end;
} Charset;

#define ASCII_CHARSET CLITERAL(Charset) { 0, 127 }
#define ASCII_VISIBLE_CHARSET CLITERAL(Charset) { 32, 127 }

typedef struct FontGlyph {
    float u1, v1;
    float u2, v2;

    int minX, minY;
    int maxX, maxY;

    int x, y;
    int width, height;

    int advanceX;
    int advanceY;

    int bearY;

    // Stores temporary character.
    char ch;
} FontGlyph;

struct FontAtlas;
struct FontAtlasPage;


typedef struct FontDrawState {
    struct FontAtlasPage* page;

    float col1[4];
    float col2[4];
    float borderColor1[4];
    float borderColor2[4];

    FontColorMode mode;

    int borderThickness;
} FontDrawState;

typedef struct TextDrawStyle {
    int x, y, z;
} TextDrawStyle;


typedef struct TTF_data {
    void* user;
    String8 fontFileBuffer;
} TTF_data;

typedef struct FontAtlasPage {
    struct FontAtlas* atlas;

    b32 active;

    Charset fullSet;
    Charset visible;
    float scale;

    // Dynamic array of glyph elements.
    /*
    size_t glyphCount;
    size_t glyphCapacity;
    FontGlyph* glyphs;
    */

    FontDrawState ds;
    FontKind fontType;

    // Texture.
    GFX_texture* texture;
    int textureSlot;

    // The image itself.
    SoftImage* image;
    int width, height;

    struct {
        unsigned char* data;
        int w, h;
    } bitmap;

    /*
    struct {
        // List of glyphs.
        //unsigned char* data;
        void* user;
    } ttf;
    */
    TTF_data ttf;

    // Minimum bounding box.
    int x1, y1;
    int x2, y2;

    // FOR PACKING: Current advance x and y;
    int advanceX, advanceY;
    int maxRowHeight;

    u32 flags;
} FontAtlasPage;

typedef struct FontAtlas {
    FontAtlasPage pages[MAX_FONT_PAGES];
    size_t nPages;
} FontAtlas;


typedef struct GlyphList {
    ARENA* glyphArena;
    FontGlyph* glyphs;
    int count;
    int capacity;
} GlyphList;


typedef struct GFX_font {
    ARENA* mainArena;
    ARENA* imageArena;
    GlyphList glyphs;

    FontAtlas atlas;
} GFX_font;



// TODO: Could probably use a state machine.



/*
   Font's draw state.
*/

extern int FontDrawSetColor(FontDrawState* state, float r, float g, float b, float a);
extern int FontDrawSetColor2(FontDrawState* state, float r, float g, float b, float a);

extern int FontDrawSetBorderColor(FontDrawState* ds, float r, float g, float b, float a);
extern int FontDrawSetBorderColor2(FontDrawState* ds, float r, float g, float b, float a);

extern int FontDrawSetMode(FontDrawState* ds, FontDrawState mode);
extern int FontDrawSetFlags(FontDrawState* ds, b32 flags);

extern int FontDrawReset(FontDrawState* ds, FontAtlasPage* page);


/*
   Glyphs.
*/

extern int GlyphInit(FontGlyph* glyph, FontAtlasPage* page,
    int originX, int originY, int width, int height, char ch);

/*
   Font.
*/

extern GFX_font* FontInit(ARENA* arena);
extern void FontTerminate(GFX_font* font);

extern FontAtlasPage* FontPageInit(GFX_font* font, int pageIndex);
extern FontAtlasPage* FontLoadBitmap(GFX_font* font, int pageIndex,
    unsigned char* bitmap, int charPixelWidth, int charPixelHeight);
extern FontAtlasPage* FontLoadBitmapDefault(GFX_font* font, int pageIndex);
extern FontAtlasPage* FontLoadTTF(GFX_font* font, int pageIndex, const char* filePath);

extern FontAtlasPage* FontPageAdd_ascii(GFX_font* font, int pageIndex);

extern GFX_texture* FontUploadTexture(GFX_font* font, int pageIndex);

/*
   Text renderer.
*/

extern int DrawTextGetSize(GFX_font* font, const char* text,
    i32 fontHeight, float spacingX, float spacingY, float scale,
    float* w, float* h);

extern int DrawTextPro(GFX_font* font, const char* text, float ox, float oy,
    int fontHeight, float spacingX, float spacingY, float scale,
    int flags, const TextDrawStyle* options);


#endif /* FONT_H_ */
