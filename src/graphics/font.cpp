#include "font.h"
#include "../base/mathf.h"
#include "../base/base_defs.h"
#include "color.h"
#include "texture.h"

#include "../asset.h"
#include "draw3d.h"
#include "../ttf/truetype_parse.h"
#include "../ttf/truetype_renderer.h"

#include <string.h>
#include <stdlib.h>

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


// TODO: This issue with memory corruption.
#define REGION_DEFAULT_CAPACITY 4
#define ATLAS_DEFAULT_WIDTH 256
#define ATLAS_DEFAULT_HEIGHT 256

static inline int IsCharVisible(char ch)
{
    return (ch >= 33 && ch <= 126);
}



static int GetNumChannels(IMAGEFORMAT format)
{
    switch (format)
    {
        case IMAGE_FORMAT_R:
            return 1;
        case IMAGE_FORMAT_RGBA:
            return 4;
        default: return 0;
    }
}
static int GetAlphaIndex(IMAGEFORMAT format)
{
    switch (format)
    {
        case IMAGE_FORMAT_R:
            return 0;
        case IMAGE_FORMAT_RGBA:
            return 3;
        default: return 0;
    }
}


/*
   Blending modes.
*/

typedef void (*BlendFunc)(unsigned char*, unsigned char*);


static unsigned char Clamp255(unsigned char x)
{
    if (x < 0) return 0;
    if (x > 255) return 255;
    return x;
}

static void BlendNormal(unsigned char* dest, unsigned char* src)
{
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = src[3];
}
static void BlendAlpha(unsigned char* dest, unsigned char* src)
{
    float alpha = src[3] / 255.0f;
    float inv_alpha = 1.0f - alpha;

    dest[0] = (unsigned char) (src[0]*alpha + dest[0]*inv_alpha);
    dest[1] = (unsigned char) (src[1]*alpha + dest[1]*inv_alpha);
    dest[2] = (unsigned char) (src[2]*alpha + dest[2]*inv_alpha);

    dest[3] = (unsigned char) (src[3] + dest[3] * inv_alpha);
}
static void BlendAdd(unsigned char* dest, unsigned char* src)
{
    dest[0] = Clamp255(dest[0] + src[0]);
    dest[1] = Clamp255(dest[1] + src[1]);
    dest[2] = Clamp255(dest[2] + src[2]);
}
static void BlendSub(unsigned char* dest, unsigned char* src)
{
    dest[0] = Clamp255(dest[0] - src[0]);
    dest[1] = Clamp255(dest[1] - src[1]);
    dest[2] = Clamp255(dest[2] - src[2]);
}
static void BlendMul(unsigned char* dest, unsigned char* src)
{
    dest[0] = Clamp255((dest[0] * src[0]) / 255);
    dest[1] = Clamp255((dest[1] * src[1]) / 255);
    dest[2] = Clamp255((dest[2] * src[2]) / 255);
}
static void BlendDiv(unsigned char* dest, unsigned char* src)
{
    dest[0] = Clamp255((dest[0] / (float) src[0]) / 255);
    dest[1] = Clamp255((dest[1] / (float) src[1]) / 255);
    dest[2] = Clamp255((dest[2] / (float) src[2]) / 255);
}


static void BlendScreen(unsigned char* dest, unsigned char* src)
{
    dest[0] = Clamp255(255 - ((255 - dest[0]) * (255 - src[0])) / 255);
    dest[1] = Clamp255(255 - ((255 - dest[1]) * (255 - src[1])) / 255);
    dest[2] = Clamp255(255 - ((255 - dest[2]) * (255 - src[2])) / 255);
}
static void BlendReplace(unsigned char* dest, unsigned char* src)
{
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = src[3];
}

static unsigned char OverlayChannel(unsigned char dest, unsigned char src)
{
    if (dest < 128) {
        return Clamp255(2 * dest * src / 255);
    } else {
        return Clamp255(255 - 2 * (255 - dest) * (255 - src) / 255);
    }
}
static void BlendOverlay(unsigned char* dest, unsigned char* src)
{
    dest[0] = OverlayChannel(dest[0], src[0]);
    dest[1] = OverlayChannel(dest[1], src[1]);
    dest[2] = OverlayChannel(dest[2], src[2]);
}



// Dispatch table.
static const BlendFunc blend_functions[IMAGE_BLEND_COUNT] = {
    BlendNormal,
    BlendAlpha,
    BlendAdd,
    BlendSub,
    BlendMul,
    BlendDiv,
    BlendScreen,
    BlendReplace,
    BlendOverlay,
};

/*
   Image API.
*/

extern CPUIMAGE* SoftImageInit(int clipX, int clipY, int clipW, int clipH, int capW, int capH)
{
    CPUIMAGE* image = (CPUIMAGE *) _MALLOC(sizeof(CPUIMAGE));
    if (image == NULL)
    {
        fprintf(stderr, "%s: Failed to allocate image. OOM?\n", __func__);
        return NULL;
    }

    image->width = capW;
    image->height = capH;

    size_t bytecount = image->width * image->height * sizeof(unsigned char) * 4;
    image->data = (unsigned char *) _MALLOC(bytecount);
    if (image->data == NULL)
    {
        fprintf(stderr, "%s: Failed to allocate buffer for image! OOM?\n", __func__);
        _FREE(image);
        return NULL;
    }
    _MEMSET(image->data, 0, bytecount);

    SoftImageSetClip(image, clipX, clipY, clipW, clipH);
    SoftImageSetBlend(image, IMAGE_BLEND_ALPHA);

    return image;
}
extern CPUIMAGE* SoftImageInitClip(int clipW, int clipH, int capW, int capH)
{
    return SoftImageInit(0, 0, clipW, clipH, capW, capH);
}
extern CPUIMAGE* SoftImageInitOrigin(int capW, int capH)
{
    return SoftImageInit(0, 0, capW, capH, capW, capH);
}


extern void SoftImageTerminate(CPUIMAGE* image)
{
    if (image == NULL)
    {
        return;
    }

    if (image->data)
    {
        _FREE(image->data);
        image->data = NULL;
    }

    _FREE(image);
    image = NULL;
}

extern void SoftImageSetClip(CPUIMAGE *image, int x, int y, int w, int h)
{
    image->clip.x = CLAMP(x, 0, image->width);
    image->clip.y = CLAMP(y, 0, image->height);
    image->clip.w = CLAMP(x + w, 0, image->width) - image->clip.x;
    image->clip.h = CLAMP(y + h, 0, image->height) - image->clip.y;
}

extern void SoftImageSetBlend(CPUIMAGE* image, IMAGE_BLENDMODE mode)
{
    if (image->blend != mode)
    {
        image->blend = mode;
    }
}



extern int SoftImageLock(CPUIMAGE* image)
{
    // TODO: Implement.
    return 0;
}
extern int SoftImageUnlock(CPUIMAGE* image)
{
    // TODO: Implement.
    return 0;
}


extern void SoftImageBlitProOptimized(
         unsigned char* dest, int cx1, int cy1, int cw1, int ch1, int destW, int destH,
    unsigned char* const src, int cx2, int cy2, int cw2, int ch2, int srcW, int srcH,
    int nChannels)
{
    int clipW = srcW, clipH = srcH;
    int ssx = 0, ssy = 0;

    int clipX = cx2;
    int clipY = cy2;

    if (cx2 < 0)
    {
        clipW += cx2;
        ssx = -cx2;
        clipX = 0;
    }
    if (cy2 < 0)
    {
        clipH += cy2;
        ssy = -cy2;
        clipY = 0;
    }

    if (clipX + clipW > cw1) clipW = cw1 - clipX;
    if (clipY + clipH > ch1) clipH = ch1 - clipY;

    if (clipW <= 0 || clipH <= 0)
    {
        return;
    }

    // Only copies rows.
    size_t bytecount = clipW * sizeof(unsigned char) * nChannels;
    for (int y = 0; y < clipH; ++y)
    {
        size_t srcRowIndex = ((ssy + y) * srcW + ssx) * nChannels;
        size_t destRowIndex = ((clipY + y) * destW + (clipX)) * nChannels;

        _MEMCPY(&dest[destRowIndex], &src[srcRowIndex], bytecount);
    }

}


extern void SoftImageBlitOptimized(CPUIMAGE* dest,
    unsigned char* const src, int srcX, int srcY, int srcW, int srcH,
    int nChannels)
{
    SoftImageBlitProOptimized(
        dest->data, dest->clip.x, dest->clip.y, dest->clip.w, dest->clip.h, dest->width, dest->height,
        src, srcX, srcY, srcW, srcH, srcW, srcH,
        nChannels
    );
}

extern void SoftImageBlitOptimized(CPUIMAGE* const src, int srcX, int srcY,
    unsigned char* dest, int destW, int destH,
    int nChannels)
{
    SoftImageBlitProOptimized(
        dest, 0, 0, destW, destH, destW, destH,
        src->data, srcX, srcY, src->clip.w, src->clip.h, src->width, src->height,
        nChannels
    );
}


    /*
    int clipW = src->width, clipH = src->height;
    int ssx = 0, ssy = 0;

    int clipX = srcX;
    int clipY = srcY;

    if (srcX < 0) {
        clipW += srcX;
        ssx = -srcX;
        clipX = 0;
    }
    if (srcY < 0) {
        clipH += srcY;
        ssy = -srcY;
        clipY = 0;
    }


    if (srcX + clipW > destW) clipW = destW - clipX;
    if (srcY + clipH > destH) clipH = destH - clipY;

    if (clipW <= 0 || clipH <= 0) {
        return;
    }

    size_t bytecount = clipW * 4;
    for (int y = 0; y < clipH; ++y) {
        size_t srcRowIndex = ((ssy + y) * src->width + ssx) * 4;
        size_t destRowIndex = ((clipY + y) * destW + (clipX)) * 4;

        _MEMCPY(&dest[destRowIndex], &src->data[srcRowIndex], bytecount);
    }
}
    */


/*
   This doesn't care about individual pixels or blending modes.
   It simply copies/replaces scanline rows.
*/

extern void SoftImageBlitOptimized(CPUIMAGE* dest, CPUIMAGE* src, int nChannels)
{
    SoftImageBlitProOptimized(
        dest->data, dest->clip.x, dest->clip.y, dest->clip.w, dest->clip.h, dest->width, dest->height,
        src->data, src->clip.x, src->clip.y, src->clip.w, src->clip.h, src->width, src->height,
        nChannels
    );
}

/*
   Normal blitting (with blending).
*/


extern void SoftImageBlitPro(
         IMAGE_BLENDMODE generalBlend,
         unsigned char* dest, int cx1, int cy1, int cw1, int ch1, int destW, int destH,
    unsigned char* const src, int cx2, int cy2, int cw2, int ch2, int srcW, int srcH, int nChannels)
{
    int clipW = srcW, clipH = srcH;
    int ssx = 0, ssy = 0;

    int clipX = cx2;
    int clipY = cy2;

    if (cx2 < 0)
    {
        clipW += cx2;
        ssx = -cx2;
        clipX = 0;
    }
    if (cy2 < 0)
    {
        clipH += cy2;
        ssy = -cy2;
        clipY = 0;
    }


    if (cx2 + clipW > destW) clipW = destW - clipX;
    if (cy2 + clipH > destH) clipH = destH - clipY;

    if (clipW <= 0 || clipH <= 0) {
        return;
    }

    BlendFunc blending = blend_functions[generalBlend];

    size_t bytecount = clipW * sizeof(unsigned char) * nChannels;
    for (int y = 0; y < clipH; ++y)
    {
        for (int x = 0; x < clipW; ++x)
        {
            size_t srcIndex = ((ssy + y) * srcW + (ssx + x)) * nChannels;
            size_t destIndex = ((clipY + y) * destW + (clipX + x)) * nChannels;

            // Get pixel from source.
            unsigned char* srcPixel = &src[srcIndex];
            unsigned char* destPixel = &dest[destIndex];

            blending(destPixel, srcPixel);
        }
    }
}

extern void SoftImageBlit(CPUIMAGE* dest, CPUIMAGE* src)
{
    SoftImageBlitPro(
        src->blend,
        dest->data, dest->clip.x, dest->clip.y, dest->clip.w, dest->clip.h, dest->width, dest->height,
        src->data, src->clip.x, src->clip.y, src->clip.w, src->clip.h, src->width, src->height,
        4
    );
}

extern int SoftImageResizeUpper(CPUIMAGE* image, int capW, int capH)
{
    if (image->width == capW && image->height == capH)
    {
        return -1;
    }
    if (capW < image->width || capH < image->height)
    {
        fprintf(stderr, "%s: Image is resized to become smaller!\n", __func__);
        return -1;
    }


    unsigned char* oldData;
    size_t oldWidth, oldHeight;

    oldData = image->data;
    oldWidth = image->width;
    oldHeight = image->height;

    size_t bytecount = capW * capH * sizeof(unsigned char) * 4;
    unsigned char* newData = (unsigned char *) _MALLOC(bytecount);
    if (newData == NULL)
    {
        fprintf(stderr, "%s: Failed to allocate memory for new data!\n", __func__);
        return -1;
    }
    _MEMSET(newData, 0, bytecount);

    // Copy rows to dest image.
    {
        size_t rowcount = oldWidth * 4;
        for (int y = 0; y < oldHeight; ++y)
        {
            size_t srcRowIndex = ((y) * oldWidth) * 4;
            size_t destRowIndex = ((y) * capW) * 4;

            _MEMCPY(&newData[destRowIndex], &oldData[srcRowIndex], rowcount);
        }
    }
    _FREE(oldData);

    image->data = newData;
    image->width = capW;
    image->height = capH;

    // Doesn't need to set clamped clipping because we assume that the image is
    // resized to grow larger.

    return 0;
}





/*
   Glyphs.
*/

static int CalculateBoundingBox(FONTGLYPH* glyph, FONTATLASPAGE* page, char ch, int* mX, int* mY, int* MX, int* MY)
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
            for (i32 y = 0; y < glyph->height; ++y)
            {
                u32 scanline_bits = page->rom.bitmapData.data[(glyph->height * idx) + glyph->height - 1 - y];

                for (i32 x = 0; x < glyph->width; ++x) {
                    u32 pixel = scanline_bits & (1 << (glyph->width - 1 - x));

                    // Position (x, y).
                    if (pixel) {
                        minX = MIN(minX, x);
                        minY = MIN(minY, y);
                        maxX = MAX(maxX, x);
                        maxY = MAX(maxY, y);
                    }
                }
            }
        } break;
        // TODO: Fix size issues, maybe.


        // These are the fonts that expect an RGBA image or a single-component image.
        case FONT_KIND_TTF:
        case FONT_KIND_OTF:
        {
            IMAGEDATA* image = &page->rom.images[idx];

            int channels = GetNumChannels(image->format);
            int alphaIndex = GetAlphaIndex(image->format);

            for (int y = 0; y < image->h; ++y)
            {
                for (int x = 0; x < image->w; ++x)
                {
                    int alphaOffset = ((y * image->w + x) * channels) + alphaIndex;

                    if (image->data[alphaOffset])
                    {
                        minX = MIN(minX, x);
                        minY = MIN(minY, y);
                        maxX = MAX(maxX, x);
                        maxY = MAX(maxY, y);
                    }
                }
            }
        } break;

        case FONT_KIND_NONE:
        default: return 0;
    }
    // Only set the AABB coordinates if there's any,
    // or if the values were modified from the initial conditions.
    if (mX && minX != UPPER_BOUND) *mX = minX;
    if (mY && minY != UPPER_BOUND) *mY = minY;
    if (MX && maxX != LOWER_BOUND) *MX = maxX;
    if (MY && maxY != LOWER_BOUND) *MY = maxY;

    return 0;
}

extern int GlyphInit(FONTGLYPH* glyph, FONTATLASPAGE* page, int originX, int originY, int width, int height, char ch)
{
    if (glyph == NULL)
    {
        fprintf(stderr, "%s: Invalid glyph!\n", __func__);
        return -1;
    }

    _MEMSET(glyph, 0, sizeof(*glyph));
    glyph->x = originX;
    glyph->y = originY;
    glyph->width = width;
    glyph->height = height;
    glyph->ch = ch;

    //fprintf(stderr, "%s: Glyph (x:%d, y:%d)\n", __func__, glyph->x, glyph->y);

    // We calculate its bounding box.
    CalculateBoundingBox(glyph, page, ch, &glyph->minX, &glyph->minY, &glyph->maxX, &glyph->maxY);

    // We calculate the advance by taking the sum of the left bearing, the glyph width and the right bearing
    i32 left = 0, right = 0;
    int bottom = 0, top = 0;

    left = glyph->minX;
    right = glyph->width - glyph->maxX;

    bottom = glyph->minY;
    top = glyph->height - glyph->maxY;

    // We then find the texture coordinates.
    glyph->u1 = (glyph->x) / (float) page->w;
    glyph->v1 = (glyph->y) / (float) page->h;
    glyph->u2 = (glyph->x + glyph->width) / (float) page->w;
    glyph->v2 = (glyph->y + glyph->height) / (float) page->h;

    //fprintf(stderr, "%s: Glyph (u1:%f, v1:%f, u2:%f, v3:%f)\n", __func__, glyph->u1, glyph->v1, glyph->u2, glyph->v2);


    int w = glyph->maxX - glyph->minX;
    int h = glyph->maxY - glyph->minY;

    // The advance X and Y are calculated from the baseline bearing.
    glyph->advanceX = left + w + right;
    glyph->advanceY = bottom + h + top;
    //glyph->advanceX = w;
    //glyph->advanceY = h;
    //glyph->advanceX = right;
    //glyph->advanceY = h;

    return 0;
}

/*
   Draw state.
*/

extern int FontDrawSetColor(FONT_DRAWSTATE* state, float r, float g, float b, float a)
{
    state->col1[0] = r;
    state->col1[1] = g;
    state->col1[2] = b;
    state->col1[3] = a;
    return 0;
}
extern int FontDrawSetColor2(FONT_DRAWSTATE* state, float r, float g, float b, float a)
{
    state->col2[0] = r;
    state->col2[1] = g;
    state->col2[2] = b;
    state->col2[3] = a;
    return 0;
}
extern int FontDrawSetBorderColor(FONT_DRAWSTATE* state, float r, float g, float b, float a)
{
    state->borderColor[0] = r;
    state->borderColor[1] = g;
    state->borderColor[2] = b;
    state->borderColor[3] = a;
    return 0;
}

extern int FontDrawSetMode(FONT_DRAWSTATE* ds, FONT_COLORMODE mode)
{
    if (ds->mode != mode) {
        ds->mode = mode;
    }
    return 0;
}
extern int FontDrawSetFlags(FONT_DRAWSTATE* ds, b32 flags)
{
    ds->page->flags |= flags;
    return 0;
}

// Resets to some sane defaults.
extern int FontDrawReset(FONT_DRAWSTATE* ds, FONTATLASPAGE* page)
{
    _MEMSET(ds, 0, sizeof(*ds));
    ds->page = page;
    ds->borderThickness = 1;

    FontDrawSetColor(ds, 1.0, 1.0, 1.0, 1);
    FontDrawSetColor2(ds, 1.0, 0.7, 0.7, 1);
    FontDrawSetBorderColor(ds, 0.1, 0.1, 0.1, 1);
    FontDrawSetMode(ds, FONT_COLOR_UD);
    FontDrawSetFlags(ds, FONT_FLAG_RELATIVE | FONT_FLAG_BORDERED);

    return 0;
}



/*
   The atlas.
*/

static int EstimateAtlasSize(FONTATLASPAGE* page, int stoppingWidth, int* atlasWidth, int* atlasHeight)
{
    int advX = 0;
    int advY = 0;
    int maxRowHeight = 0;

    for (int i = page->visible.begin; i < page->visible.end; ++i)
    {
        float x, y;
        FONTGLYPH glyph;

        x = advX; y = advY;
        GlyphInit(&glyph, page, x, y, page->rom.bitmapData.w, page->rom.bitmapData.h, i);

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

    fprintf(stderr, "%s: Atlas width: %d, atals height: %d\n", __func__, *atlasWidth, *atlasHeight);

    return 0;
}

static int UpdateRegionFit(FONTATLASPAGE* page)
{
    i32 minX, minY;
    i32 maxX, maxY;
    minX = minY = 10000;
    maxX = maxY = -10000;

    for (int i = 0; i < page->glyphCount; ++i) {
        i32 mx = page->glyphs[i].minX;
        i32 my = page->glyphs[i].minY;
        i32 Mx = page->glyphs[i].maxX;
        i32 My = page->glyphs[i].maxY;

        if (maxX < Mx) maxX = Mx;
        if (maxY < My) maxY = My;

        if (minX > mx) minX = mx;
        if (minY > my) minY = my;
    }

    // Then we update the metrics in the regions,
    // and then in the atlas, if there's any.
    page->x1 = minX;
    page->y1 = minY;
    page->x2 = maxX;
    page->y2 = maxY;

    // TODO: Resizing?

    return 0;
}


/*
   Debugging.
*/

static int PrintBitmap(CPUIMAGE* image)
{
    fprintf(stderr, "BITMAP: \n");
    for (int y = 0; y < image->height; ++y)
    {
        for (int x = 0; x < image->width; ++x)
        {
            unsigned char* pixel = &image->data[(y * image->width + x) * 4];
            if (pixel[3]) {
                printf("#");
            } else {
                printf(".");
            }
        }

        printf("\n");
    }
    return 0;
}

/*
   The glyph software rendering pipeline.
*/

static int RasterBitmap(CPUIMAGE* dest, char ch,
    unsigned char* bitmap, FONTGLYPH* const glyph)
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
                //u8 val = ((y / (float) glyph->height) + (x / (float) glyph->width)) * 0.5 * 255;
                //u8 val = 0xff;

                pixel[0] = 0xff;
                pixel[1] = 0xff;
                pixel[2] = 0xff;
                pixel[3] = 0xff;
            }
        }
    }

    return 0;
}

static inline void ApplyColorPixel(FONTATLASPAGE* page, FONTGLYPH* const glyph,
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

// Beofre copying to the main atlas.
static int ColorizeImage(FONTATLASPAGE* page, FONTGLYPH* const glyph, CPUIMAGE* dest)
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

static int TranslateImage(CPUIMAGE* dest, CPUIMAGE* src,
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


static int FlipImageY(CPUIMAGE* dest)
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
static int ExpandImage(FONTATLASPAGE* page, FONTGLYPH* const glyph, int borderThickness,
    CPUIMAGE* dest, CPUIMAGE* src
) {
    /*
    if (glyph->minX <= 0 || glyph->minY <= 0 || glyph->maxX <= 0 || glyph->maxY <= 0) {
        return -1;
    }
    */

    //int width = glyph->width + (borderThickness * 2);
    //int height = glyph->height + (borderThickness * 2);

    // Use border color.
    unsigned char r = page->ds.borderColor[0] * 255.0f;
    unsigned char g = page->ds.borderColor[1] * 255.0f;
    unsigned char b = page->ds.borderColor[2] * 255.0f;
    unsigned char a = page->ds.borderColor[3] * 255.0f;

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
                        //if (dx == 0 || dy == 0) continue;

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

static int UpdatePageStep(FONTATLASPAGE* page, FONTGLYPH* glyph)
{
    // Update + add glyph.
    page->x1 = MIN(page->x1, glyph->x);
    page->y1 = MIN(page->y1, glyph->y);
    page->x2 = MAX(page->x2, glyph->x + glyph->width);
    page->y2 = MAX(page->y2, glyph->y + glyph->height);


    return 0;
}


static int AddGlyphToPage(FONTATLASPAGE* page, FONTGLYPH* glyph)
{
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

    CPUIMAGE *tmp1 = SoftImageInitClip(clipW, clipH, capW, capH),
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

    SoftImageBlitOptimized(tmp3, glyph->x, glyph->y, page->data, page->w, page->h, 4);

    SoftImageTerminate(tmp1);
    SoftImageTerminate(tmp2);
    SoftImageTerminate(tmp3);


    page->glyphs[page->glyphCount++] = *glyph;

    return 0;
}



// Returns the page at the specific page index.
extern FONTATLASPAGE* FontInitPage(FONT* font, int pageIndex, FONTKIND type)
{
    if (pageIndex < 0 || pageIndex >= MAX_FONT_PAGES)
    {
        fprintf(stderr, "%s: Couldn't initialize page. Out of bounds access.\n", __func__);
        return NULL;
    }
 
    FONTATLASPAGE* page = &font->atlas.pages[pageIndex];

    // Load page with default width.
    {
        int width  = ATLAS_DEFAULT_WIDTH;
        int height = ATLAS_DEFAULT_HEIGHT;
        u8* data;

        size_t bytecount = (width * 4) * (height * 4) * sizeof(unsigned char) * 4;
        data = (u8 *) _MALLOC(bytecount);
        if (data == NULL)
        {
            fprintf(stderr, "%s: Failed to load font atlas image! OOM?\n", __func__);
            return NULL;
        }
        // We set the atlas pixels to 0 (fully transparent black).
        _MEMSET(data, 0, bytecount);

        // Then init the page with some default data.
        _MEMSET(page, 0, sizeof(*page));
        page->atlas = &font->atlas;
        page->data = data;

        page->fontType = type;
        page->w = ATLAS_DEFAULT_WIDTH;
        page->h = ATLAS_DEFAULT_HEIGHT;

        // Prepare texture slot.
        page->texture = NULL;
        font->textureSlot = 0;

        page->advanceX = 0;
        page->advanceY = 0;
        page->maxRowHeight = 0;
        page->flags = FONT_FLAG_NONE;
    }

    // Init ASCII charset.
    {
        page->fullSet = ASCII_CHARSET;
        page->visible = ASCII_VISIBLE_CHARSET;
    }

    // Load draw state.
    {
        FONT_DRAWSTATE* ds = &page->ds;
        FontDrawReset(ds, page);
    }

    if (pageIndex >= font->atlas.nPages)
    {
        font->atlas.nPages = pageIndex + 1;
    }

    return page;
}


// Char pixel width is in bits (8 bits = 1 byte).
// charPixelHeight is 13 for default.
extern FONT* LoadFontBitmap(unsigned char* bitmap, int charPixelWidth, int charPixelHeight)
{
    FONT* font = (FONT *) _MALLOC(sizeof(FONT));
    if (font == NULL)
    {
        fprintf(stderr, "%s: Failed to allocate font handle. OOM?\n", __func__);
        return NULL;
    }
    _MEMSET(font, 0, sizeof(*font));
    font->atlas.nPages = 0;


    // Init default page.
    FONTATLASPAGE* page = FontInitPage(font, FONT_PAGE_REGULAR, FONT_KIND_BITMAP);

    // Initialize bitmap data.
    {
        page->rom.bitmapData.format = IMAGE_FORMAT_R;
        page->rom.bitmapData.data = bitmap;
        page->rom.bitmapData.w = charPixelWidth;
        page->rom.bitmapData.h = charPixelHeight;

    }

    // Atlas size estimation.
    {
        EstimateAtlasSize(page, ATLAS_DEFAULT_WIDTH * 2, &page->w, &page->h);
    }

    // Populate region with ASCII charset.
    for (int i = page->visible.begin; i < page->visible.end; ++i)
    {
        FONTGLYPH glyph;
        GlyphInit(&glyph, page, page->advanceX, page->advanceY, page->rom.bitmapData.w, page->rom.bitmapData.h, i);
        AddGlyphToPage(page, &glyph);
        /*
        {
            EstimateAtlasSize(page, ATLAS_DEFAULT_WIDTH, &page->w, &page->h);
        }
        */

        page->advanceX += glyph.width;
        page->maxRowHeight = MAX(page->maxRowHeight, glyph.height);

        if (page->advanceX + 2 >= page->w)
        {
            page->advanceX = 0;
            page->advanceY += page->maxRowHeight;
            page->maxRowHeight = glyph.height;
        }
    }
    font->atlas.nPages ++;

    return font;
}
extern FONT* LoadDefaultFont(void)
{
    return LoadFontBitmap((unsigned char *) bitmap_glyphs, 8, 13);
}

extern FONT* LoadFontTTF(const char* filePath)
{
    // First, load the file then the TTF font info.
    STRING8 buf = AssetLoad(filePath);

    TT_FONTINFO fontinfo;
    TT_FontInit(buf, &fontinfo);
    if (!fontinfo.initialized) {
        fprintf(stderr, "%s: Failed to load font!\n", __func__);
        return NULL;
    }

    FONT* font = (FONT *) _MALLOC(sizeof(FONT));
    if (font == NULL) {
        fprintf(stderr, "%s: Failed to allocate TTF font. OOM?\n", __func__);
        return NULL;
    }
    _MEMSET(font, 0, sizeof(*font));
    font->atlas.nPages = 0;

    // Initialize the stack. (64 MB)
    StackInit(&font->stack, 0x4000000);



    // Init default page.
    // FONT_PAGE_REGULAR has the value 0.
    FONTATLASPAGE* page = FontInitPage(font, FONT_PAGE_REGULAR, FONT_KIND_TTF);

    // Initialize bitmap data.

    // TODO: make.
    {
        // Load 256 images.
        size_t bytes = sizeof(IMAGEDATA) * 256;
        IMAGEDATA* imageBuffer = (IMAGEDATA *) _MALLOC(bytes);
        if (imageBuffer == NULL)
        {
            fprintf(stderr, "%s: Couldn't load image buffer!\n", __func__);
            return font;
        }
        _MEMSET(imageBuffer, 0, bytes);
        page->rom.images = imageBuffer;
        page->rom.glyphCount = 0;
        page->rom.glyphCapacity = 256;


        for (int i = page->visible.begin; i <= page->visible.end; ++i)
        {
            // Load ASCII characters.
            int idx = i - ' ';
            u32 codepoint = i;
            IMAGEDATA* image = &page->rom.images[idx];

            int glyphIndex = TT_GlyphIndex(&fontinfo, codepoint);
            //fprintf(stderr, "%s: glyph index: %d, codepoint: %d (' %c ')\n", __func__, glyphIndex, codepoint, codepoint);

            // TODO: OK.
            int xoff = 0, yoff = 0;
            float scale = TT_ScaleForPixelHeight(buf, &fontinfo, 15);

            image->data = TT_RenderGlyph_SDF(&fontinfo, scale*0.01f, glyphIndex, 5, 180, 36, &image->w, &image->h, &xoff, &yoff);
            image->format = IMAGE_FORMAT_RGBA;


            //fprintf(stderr, "%s: image data, w, h: (%p, %d, %d)\n\n", __func__, image->data, image->w, image->h);

            //CPUIMAGE img;
            //img.data = image->data;
            //img.width = image->w;
            //img.height = image->h; // Huh?
            //PrintBitmap(&img);
        }

        page->rom.bitmapData.w = 1;
        page->rom.bitmapData.h = 1;
    }


    // Atlas size estimation.
    {
        EstimateAtlasSize(page, ATLAS_DEFAULT_WIDTH * 4, &page->w, &page->h);
    }

    // Populate region with ASCII charset.
    for (int i = page->visible.begin; i < page->visible.end; ++i)
    {
        int idx = i - ' ';
        IMAGEDATA* img = &page->rom.images[idx];

        //fprintf(stderr, "%s: img w:%d, img h:%d\n", __func__, img->w, img->h);

        FONTGLYPH glyph;
        GlyphInit(&glyph, page, page->advanceX, page->advanceY, img->w, img->h, i);
        AddGlyphToPage(page, &glyph);

        //{
        //    EstimateAtlasSize(page, ATLAS_DEFAULT_WIDTH, &page->w, &page->h);
        //}

        page->advanceX += glyph.width;
        page->maxRowHeight = MAX(page->maxRowHeight, glyph.height);

        if (page->advanceX + 2 >= page->w) {
            page->advanceX = 0;
            page->advanceY += page->maxRowHeight;
            page->maxRowHeight = glyph.height;
        }
    }

    font->atlas.nPages ++;

    // We remove the TTF buffer afterwards.
    if (buf.data)
    {
        _FREE(buf.data);
    }

    if (page->fontType != FONT_KIND_BITMAP)
    {
        // Remove each image.
        for (int i = 0; i < page->rom.glyphCount; ++i)
        {
            IMAGEDATA* img = &page->rom.images[i];

            if (img->data)
            {
                _FREE(img->data);
            }
            img->data = NULL;
        }

        if (page->rom.images)
        {
            _FREE(page->rom.images);
        }
        page->rom.images = NULL;
    }


    fprintf(stderr, "%s: Page width: %d, page height:%d\n", __func__, page->w, page->h);


    return font;
}


extern void FontPageTerminate(FONTATLASPAGE* page)
{
    // Remove images, if there's any.
    if (page->fontType != FONT_KIND_BITMAP)
    {
        // Remove each image.
        for (int i = 0; i < page->rom.glyphCount; ++i)
        {
            IMAGEDATA* img = &page->rom.images[i];

            if (img->data) _FREE(img->data);
            img->data = NULL;
        }

        if (page->rom.images) _FREE(page->rom.images);
        page->rom.images = NULL;
    }

    // Remove glyphs.
    if (page->glyphs) _FREE(page->glyphs);
    page->glyphs = NULL;
    page->atlas = NULL;

    // Remove texture.
    if (page->texture)
    {
        TextureTerminate(page->texture);
    }

    // Remove atlas.
    if (page->data) _FREE(page->data);
    page->data = NULL;
}

extern void FontTerminate(FONT* font)
{
    if (font == NULL)
    {
        return;
    }

    // Remove slices.
    for (int i = 0; i < font->atlas.nPages; ++i)
    {
        FONTATLASPAGE* page = &font->atlas.pages[i];
        FontPageTerminate(page);
    }
    // TODO: Might be risky.
    StackTerminate(&font->stack);

    // Cleanup anything else.
    _MEMSET(font, 0, sizeof(*font));

    _FREE(font);
}

extern void FontPageReleaseTexture(FONT* font, int id)
{
    FONTATLASPAGE* page = &font->atlas.pages[id];
    if (page == NULL)
    {
        return;
    }

    if (page->texture)
    {
        TextureTerminate(page->texture);
    }
}


/*
extern float DrawFontGetTextWidth(FONT *font, const char *text, i32 fontHeight, float scale)
{
    //font->atlas.regions[0]
}
*/

extern TEXTURE* UploadFont(FONT* font, int pageIndex)
{
    FONTATLASPAGE* page = &font->atlas.pages[pageIndex];

    fprintf(stderr, "%s: upload font: page w h data: %d %d %p", __func__, page->w, page->h, page->data);

    TEXTURE* tex = LoadImmutableTextureFromPixels(page->w, page->h, page->data, 0);
    page->texture = tex;

    return tex;
}


static const TEXTDRAWSTYLE defaultStyle = CLITERAL(TEXTDRAWSTYLE) {
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
};

extern int DrawTextPro(FONT* font, const char* text, float ox, float oy,
    i32 fontHeight, float spacingX, float spacingY, float scale,
    int flags, const TEXTDRAWSTYLE* options)
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

    FONTATLASPAGE* page = &font->atlas.pages[FONT_PAGE_REGULAR];

    FONTGLYPH* glyph;
    float currentX, currentY;
    float maxHeight;
    int idx;

    // TODO: Add some text memory boundaries verification.
    idx = *text - ' ';
    glyph = &page->glyphs[idx];

    currentX = ox;
    currentY = oy;
    maxHeight = glyph->height;

    DrawMatIdentity();
    DrawTexture(font->textureSlot, page->texture);

    const char* ptr = text;
    for (; *ptr; ++ptr)
    {
        if (*ptr == '\n')
        {
            // Newline character.
            currentY -= (maxHeight + spacingY) * scale;
            maxHeight = 0;
            currentX = ox;
            continue;
        }

        idx = *ptr - ' ';
        glyph = &page->glyphs[idx];

        //float width = (glyph->maxX - glyph->minX);
        //float height = (glyph->maxY - glyph->minY);
        float width = glyph->width;
        float height = glyph->height;

        //fprintf(stderr, "%s: Glyph (u1:%f, v1:%f, sx:%f, sy:%f)\n", __func__, glyph->u1, glyph->v1, (glyph->u2 - glyph->u1), (glyph->v2 - glyph->v1));
        DrawSrcRect(glyph->u1, glyph->v1, (glyph->u2 - glyph->u1), (glyph->v2 - glyph->v1));
        //DrawSrcRect(glyph->u1, glyph->v1, 0.01f, 0.01f);

        DrawMatIdentity();
        DrawMatTranslate3D(currentX, currentY, 0);
        DrawRect(width * scale, height * scale);
        currentX += (glyph->advanceX + spacingX) * scale;
        maxHeight = MAX(maxHeight, height);
    }
    // Restore previous state.
    DrawSrcRect(0, 0, 1, 1);
    DrawMatTranslate3D(0, 0, 0);

    return 0;

    // Ok.
    //DrawTexture(0, NULL);

    //return 0;
}

