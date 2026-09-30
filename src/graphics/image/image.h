#ifndef IMAGE_H_
#define IMAGE_H_ 1

#include "../../mem/arena.h"

typedef enum ImageBlendMode {
    IMAGE_BLEND_NONE = 0,

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
} ImageBlendMode;

typedef enum ImageFormat {
    IMAGE_FORMAT_R,
    IMAGE_FORMAT_RGBA,
} ImageFormat;

typedef struct SoftImage {
    unsigned char* data;
    int width, height;
    int nChannels;

    struct {
        int x, y;
        int w, h;
    } clip;

    ImageBlendMode blend;
} SoftImage;

extern SoftImage* SoftImageInitEx(
    ARENA* arena,
    int clipX, int clipY,
    int clipW, int clipH,
    int capW, int capH, int nChannels);

extern SoftImage* SoftImageInitClip(
    ARENA* arena,
    int clipW, int clipH,
    int capW, int capH, int nChannels);

extern SoftImage* SoftImageInitOrigin(ARENA* arena, int capW, int capH, int nChannels);

extern void SoftImageTerminate(SoftImage* image);

// Resize an image to a larger size.
extern SoftImage* SoftImageResizeUpper(ARENA* arena, SoftImage* image, int capW, int capH);

extern void SoftImageSetClip(SoftImage* image, int x, int y, int w, int h);
extern void SoftImageSetBlend(SoftImage* image, ImageBlendMode mode);

extern int SoftImageLock(SoftImage* image);
extern int SoftImageUnlock(SoftImage* image);

/*
   Optimized version (no blending).
*/

extern void SoftImageBlitProOptimized(
         unsigned char* dest, int cx1, int cy1, int cw1, int ch1, int destW, int destH,
    unsigned char* const src, int cx2, int cy2, int cw2, int ch2, int srcW, int srcH,
    int nChannels);

extern void SoftImageBlitOptimized(SoftImage* dest, SoftImage* src, int nChannels);
extern void SoftImageBlitOptimized(SoftImage* dest,
    unsigned char* const src, int srcX, int srcY, int srcW, int srcH, int nChannels);
extern void SoftImageBlitOptimized(SoftImage* const src, int srcX, int srcY,
    unsigned char* dest, int destW, int destH, int nChannels);

/*
   Normal versions (with blending).
*/

extern void SoftImageBlitPro(
    ImageBlendMode generalBlend,
         unsigned char* dest, int cx1, int cy1, int cw1, int ch1, int destW, int destH,
    unsigned char* const src, int cx2, int cy2, int cw2, int ch2, int srcW, int srcH, int nChannels);

extern void SoftImageBlit(SoftImage* dest, SoftImage* src);

/*
   Some procedural image generation techniques.
*/

// Uses BIOS VGA 12h mode pallete.
extern void SoftImage_generateGlitchBIOS(SoftImage* dest, float rowChance);






#endif /* IMAGE_H_ */
