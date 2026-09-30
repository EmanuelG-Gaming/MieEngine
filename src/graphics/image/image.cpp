#include "image.h"
#include "../../base/base_defs.h"
#include "../../base/math/base_rng.h"

#include <string.h>
#include <stdio.h>

static u8 VGA12h_pallette[3*16] = {
    // "Dark" colors.
    0, 0, 0,
    0, 0, 170,
    0, 170, 0,
    0, 170, 170,
    170, 0, 0,
    170, 0, 170,
    170, 85, 0,
    170, 170, 170,
    85, 85, 85,

    // "Bright" colors.
    85, 85, 255,
    85, 255, 85,
    85, 255, 255,
    255, 85, 85,
    255, 85, 255,
    255, 255, 85,
    255, 255, 255,
};


static int GetNumChannels(ImageFormat format)
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
static int GetAlphaIndex(ImageFormat format)
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
ALIGN_DECL(64) static const BlendFunc blend_functions[IMAGE_BLEND_COUNT] = {
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



extern SoftImage* SoftImageInitEx(
    ARENA* arena,
    int clipX, int clipY,
    int clipW, int clipH,
    int capW, int capH, int nChannels)
{
    SoftImage* image = ArenaPushStruct(arena, SoftImage);
    image->width = capW;
    image->height = capH;
    image->nChannels = nChannels;

    size_t bytecount = image->width * image->height * nChannels;
    image->data = ArenaPushArrayZero(arena, unsigned char, bytecount);

    SoftImageSetClip(image, clipX, clipY, clipW, clipH);
    SoftImageSetBlend(image, IMAGE_BLEND_ALPHA);

    return image;
}


extern SoftImage* SoftImageInitClip(
    ARENA* arena,
    int clipW, int clipH,
    int capW, int capH, int nChannels)
{
    return SoftImageInitEx(arena, 0, 0, clipW, clipH, capW, capH, nChannels);
}
extern SoftImage* SoftImageInitOrigin(ARENA* arena, int capW, int capH, int nChannels)
{
    return SoftImageInitEx(arena, 0, 0, capW, capH, capW, capH, nChannels);
}


extern void SoftImageTerminate(SoftImage* image)
{
    // We don't free the image tho.
}


extern void SoftImageSetClip(SoftImage* image, int x, int y, int w, int h)
{
    image->clip.x = CLAMP(x, 0, image->width);
    image->clip.y = CLAMP(y, 0, image->height);
    image->clip.w = CLAMP(x+w, 0, image->width) - image->clip.x;
    image->clip.h = CLAMP(y+h, 0, image->height) - image->clip.y;
}

extern void SoftImageSetBlend(SoftImage* image, ImageBlendMode mode)
{
    if (image->blend != mode)
    {
        image->blend = mode;
    }
}

extern int SoftImageLock(SoftImage* image)
{
    // TODO: Implement.
    return 0;
}
extern int SoftImageUnlock(SoftImage* image)
{
    // TODO: Implement.
    return 0;
}


// The problem with this function
// is that it doesn't free the memory in the arena.
// You'd probably have to use a temporary scratch arena
// to output the finished, resized image to the font atlas.
extern SoftImage* SoftImageResizeUpper(ARENA* arena, SoftImage* image, int capW, int capH)
{
    if (image->width == capW && image->height == capH)
    {
        return NULL;
    }
    if (capW < image->width || capH < image->height)
    {
        //fprintf(stderr, "%s: Image is resized to become smaller!\n", __func__);
        return NULL;
    }


    unsigned char* oldData;
    size_t oldWidth, oldHeight;

    oldData = image->data;
    oldWidth = image->width;
    oldHeight = image->height;

    SoftImage* out = ArenaPushStruct(arena, SoftImage);

    size_t bytecount = capW * capH * 4;
    unsigned char* newData = ArenaPushArrayZero(arena, unsigned char, bytecount);

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
    //_FREE(oldData);

    out->data = newData;
    out->width = capW;
    out->height = capH;

    SoftImageSetClip(out, image->clip.x, image->clip.y, image->clip.w, image->clip.h);
    SoftImageSetBlend(out, image->blend);

    return 0;
}



/*
   Image blitting.
*/

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


extern void SoftImageBlitOptimized(SoftImage* dest,
    unsigned char* const src, int srcX, int srcY, int srcW, int srcH,
    int nChannels)
{
    SoftImageBlitProOptimized(
        dest->data, dest->clip.x, dest->clip.y, dest->clip.w, dest->clip.h, dest->width, dest->height,
        src, srcX, srcY, srcW, srcH, srcW, srcH,
        nChannels
    );
}

extern void SoftImageBlitOptimized(SoftImage* const src, int srcX, int srcY,
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
   This doesn't care about individual pixels or blending modes.
   It simply copies/replaces scanline rows.
*/

extern void SoftImageBlitOptimized(SoftImage* dest, SoftImage* src, int nChannels)
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
         ImageBlendMode generalBlend,
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

extern void SoftImageBlit(SoftImage* dest, SoftImage* src)
{
    SoftImageBlitPro(
        src->blend,
        dest->data, dest->clip.x, dest->clip.y, dest->clip.w, dest->clip.h, dest->width, dest->height,
        src->data, src->clip.x, src->clip.y, src->clip.w, src->clip.h, src->width, src->height,
        4
    );
}

/*
   Procedural image generation.
*/

extern void SoftImage_generateGlitchBIOS(SoftImage* dest, float rowChance)
{
    int width = dest->width;
    int height = dest->height;

    RNG_seed(0x569235446342381ULL, 45356734879ULL);

    for (int y = 0; y < height; ++y)
    {
        if (RNG_chance(rowChance))
        {
            for (int x = 0; x < width; ++x)
            {
                u32 palIdx = RNG_boundedRand(16);
                if (palIdx == 0)
                {
                    continue;
                }

                u8* color = VGA12h_pallette + (palIdx*3);
                unsigned char* destPixel = dest->data + ((y * width + x)*4);

                destPixel[0] = color[0];
                destPixel[1] = color[1];
                destPixel[2] = color[2];
                destPixel[3] = 255;
            }
        }
    }
}
