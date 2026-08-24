#ifndef COLOR_H_
#define COLOR_H_ 1

#include "../base/base_defs.h"
#include "../base/mathf.h"
#include "math.h"


/*
   Color theory.
*/

/*
typedef union TC_RGBA {
    struct {
        u8 r, g, b, a;
    };
    u32 hex;
    u8 val[4];
} TC_RGBA;
#define TC_DEF(r, g, b, a) CLITERAL(TC_RGBA32) { r, g, b, a }
*/

/*
   Color has RGBA components in the range [0, 1] for hardware-accelerated graphics libraries.
*/

typedef union COLOR {
    struct {
        float r, g, b, a;
    };
    float val[4];
    vec4 v;
} COLOR;

typedef struct HSV {
    float h, s, v;
} HSV;

#define COLOR_DEF(r, g, b, a) CLITERAL(COLOR) { (r) / 255.0f, (g) / 255.0f, (b) / 255.0f, (a) / 255.0f }

#define RED COLOR_DEF(255, 0, 0, 255)
#define GREEN COLOR_DEF(0, 255, 0, 255)
#define BLUE COLOR_DEF(0, 0, 255, 255)

#define CYAN COLOR_DEF(0, 255, 255, 255)
#define MAGENTA COLOR_DEF(255, 0, 255, 255)
#define YELLOW COLOR_DEF(255, 255, 0, 255)

#define BLACK COLOR_DEF(0, 0, 0, 255)
#define WHITE COLOR_DEF(255, 255, 255, 255)

#define MID_GRAY COLOR_DEF(127, 127, 127, 255)
#define DARK_GRAY COLOR_DEF(50, 50, 50, 255)
#define LIGHT_GRAY COLOR_DEF(200, 200, 200, 255)

#define TEAL COLOR_DEF(0, 127, 127, 1)

#define BLACK_TRANSPARENT COLOR_DEF(0, 0, 0, 0)
#define WHITE_TRANSPARENT COLOR_DEF(255, 255, 255, 0)

/*
   Color temperatures.
*/

#define KELVIN_CANDLE_LIGHT 1900
#define KELVIN_TUNGSTEN_40W 2400
// Standard for home illumination.
#define KELVIN_WARM_WHITE_LED 2700
#define KELVIN_SOFT_WHITE 3000
// Office illumination.
#define KELVIN_COOL_WHITE 4000
// Sun at noon.
#define KELVIN_DAYLIGHT 5000
// Cloudy day illumination. It's also used in monitors
// that cause eye strain, or in fish tanks.
#define KELVIN_DAYLIGHT_BLUISH 6500



/*
static inline TC_RGBA* TrueColorInitHex(TC_RGBA* color, u32 hex)
{
    // Need to be more careful with the endian.
    color->hex = hex;
    return color;
}
static inline TC_RGBA* TrueColorInitRGBA(TC_RGBA* color, u8 r, u8 g, u8 b, u8 a)
{
    color->r = r;
    color->g = g;
    color->b = b;
    color->a = a;

    return color;
}
static inline TC_RGBA* TrueColorInitBGRA(TC_RGBA* color, u8 b, u8 g, u8 r, u8 a)
{
    color->b = b;
    color->g = g;
    color->r = r;
    color->a = a;

    return color;
}
static inline TC_RGBA* TrueColorInitArray(TC_RGBA* color, u8* arr)
{
    color->r = arr[0];
    color->g = arr[1];
    color->b = arr[2];
    color->a = arr[3];

    return color;
}

static inline TC_RGBA* TrueColorSetOpacity(TC_RGBA* color, u8 alpha)
{
    color->a = alpha;
    return color;
}
*/

/*
   Color hexadecimal unpacking.
   Returns color components in the range [0, 255].
*/

// Format: 0xRRGGBB.
static inline COLOR* ColorUnpackRGB(COLOR* color, u32 hex)
{
    float factor = (1 / 255.0f);
    color->r = ((hex >> 16) & 0xff) * factor;
    color->g = ((hex >> 8) & 0xff) * factor;
    color->b = (hex & 0xff) * factor;
    //color->a = 1.0f;
    return color;
}
// Format: 0xBBGGRR.
static inline COLOR* ColorUnpackBGR(COLOR* color, u32 hex)
{
    float factor = (1 / 255.0f);
    color->r = (hex & 0xff) * factor;
    color->g = ((hex >> 8 ) & 0xff) * factor;
    color->b = ((hex >> 16) & 0xff) * factor;
    //color->a = 1.0f;
    return color;
}
// Format: 0xRRGGBBAA.
static inline COLOR* ColorUnpackRGBA(COLOR* color, u32 hex)
{
    float factor = (1 / 255.0f);
    color->r = ((hex >> 24) & 0xff) * factor;
    color->g = ((hex >> 16) & 0xff) * factor;
    color->b = ((hex >> 8 ) & 0xff) * factor;
    color->a = (hex         & 0xff) * factor;
    return color;
}

// Format: 0xBBGGRRAA.
static inline COLOR* ColorUnpackBGRA(COLOR* color, u32 hex)
{
    float factor = (1 / 255.0f);
    color->r = ((hex >> 8 ) & 0xff) * factor;
    color->g = ((hex >> 16) & 0xff) * factor;
    color->b = ((hex >> 24) & 0xff) * factor;
    color->a = ((hex)       & 0xff) * factor;
    return color;
}

static inline COLOR* ColorUnpackRGB565(COLOR* color, u16 hex)
{
    color->r = ((hex & 0xF800) >> 11) / 31.0f;
    color->g = ((hex & 0x07E0) >> 5 ) / 63.0f;
    color->b = ((hex & 0x001F)      ) / 31.0f;
    return color;
}
static inline COLOR* ColorUnpackRGBA4444(COLOR* color, u16 hex)
{
    color->r = ((hex & 0xF000) >> 12) / 15.0f;
    color->g = ((hex & 0x0F00) >> 8 ) / 15.0f;
    color->b = ((hex & 0x00F0) >> 4 ) / 15.0f;
    color->a = ((hex & 0x000F)      ) / 15.0f;
    return color;
}


/*
   Color packing.
*/

// 0x00LLAA.
static inline u32 ColorLuminanceAlpha(float luminance, float alpha)
{
    return ((int) (luminance * 255.0f) << 8) | (int) (alpha * 255);
}


// Non-standard color formats.
static inline u32 ColorPackRGB565(COLOR* color)
{
    return ((int)(color->r * 31) << 11) | ((int) (color->g * 63) << 5) | (int)(color->b * 31);
}
static inline u32 ColorPackRGBA4444(COLOR* color)
{
    return ((int) (color->r * 15) << 12) | ((int) (color->g * 15) << 8) | ((int) (color->b * 15) << 4) | (int)(color->a * 15);
}
static inline u32 ColorPackRGB888(COLOR* color)
{
    return ((int) (color->r * 255) << 16) | ((int) (color->g * 255) << 8) | (int)(color->b * 255);
}




// Format: 0xRRGGBBAA.
static inline u32 ColorPackRGBA(COLOR* color)
{
    return (( (((int) (color->r * 255.0f) & 0xff) << 24) | (((int) (color->g * 255.0f) & 0xff) << 16) |
              (((int) (color->b * 255.0f) & 0xff) << 8) | (((int) (color->a * 255.0f) & 0xff)) ));
}
// Format: 0xRRGGBB.
static inline u32 ColorPackRGB(COLOR* color)
{
    return (( (((int) (color->r * 255.0f) & 0xff) << 16) | (((int) (color->g * 255.0f) & 0xff) << 8) |
              (((int) (color->b * 255.0f) & 0xff)) ));
}
// Format: 0xBBGGRRAA.
static inline u32 ColorPackBGRA(COLOR* color)
{
    return (( (((int) (color->r * 255.0f) & 0xff) << 8) | (((int) (color->g * 255.0f) & 0xff) << 16) |
              (((int) (color->b * 255.0f) & 0xff) << 24) | (((int) (color->a * 255.0f) & 0xff)) ));
}
// Format: 0xBBGGRR.
static inline u32 ColorPackBGR(COLOR* color)
{
    return (( (((int) (color->r * 255.0f) & 0xff)) | (((int) (color->g * 255.0f) & 0xff) << 8) |
              (((int) (color->b * 255.0f) & 0xff) << 16) ));
}




// Normalizes RGB from [0, 255] interval into [0, 1].
static inline COLOR* ColorSDRtoHDR(COLOR* out)
{
    float factor = (1 / 255.0f);
    out->r = out->r * factor;
    out->g = out->g * factor;
    out->b = out->b * factor;
    return out;
}

static inline COLOR* ColorSetIntensity(COLOR* out, float intensity)
{
    out->r = out->r * intensity;
    out->g = out->g * intensity;
    out->b = out->b * intensity;
    return out;
}
static inline COLOR* ColorSetOpacity(COLOR* color, float alpha)
{
    color->a = alpha;
    return color;
}


static inline void ColorSDRtoHDR(COLOR* dest, float r, float g, float b, float a, float intensity)
{
    float factor = (1 / 255.0f) * intensity;

    dest->r = r * factor;
    dest->g = g * factor;
    dest->b = b * factor;
    dest->a = a * factor;
}


static inline COLOR* ColorComplementary(COLOR* out, const COLOR* src, float intensity)
{
    out->r = intensity - src->r;
    out->g = intensity - src->g;
    out->b = intensity - src->b;
    out->a = src->a;
    return out;
}

/*
   Color gamuts.
*/

// This splits the color wheel into 6 circle sectors,
// where colors get interpolated, based on these intervals.
static COLOR* HDR_HSVtoRGB(COLOR* rgb, float h, float s, float v, float intensity)
{
    float c, x, m;
    float hprime, p, q, t;

    hprime = h / 60.0f;
    if (hprime >= 6.0f) hprime = 0.0f;

    // Chroma (intensity of a color).
    c = v * s;
    x = c * (1.0f - fabs(fmod(hprime, 2.0f) - 1.0f));

    // M (value) component.
    m = v - c;

    // Sector mapping.
    if (hprime >= 0.0f && hprime < 1.0f) {
        rgb->r = c;
        rgb->g = x;
        rgb->b = 0;
    } else if (hprime >= 1.0f && hprime < 2.0f) {
        rgb->r = x;
        rgb->g = c;
        rgb->b = 0;
    } else if (hprime >= 2.0f && hprime < 3.0f) {
        rgb->r = 0;
        rgb->g = c;
        rgb->b = x;
    } else if (hprime >= 3.0f && hprime < 4.0f) {
        rgb->r = 0;
        rgb->g = x;
        rgb->b = c;
    } else if (hprime >= 4.0f && hprime < 5.0f) {
        rgb->r = x;
        rgb->g = 0;
        rgb->b = c;
    } else {
        rgb->r = c;
        rgb->g = 0;
        rgb->b = x;
    }

    float factor = (1 / 255.0f) * intensity;
    rgb->r = (rgb->r + m) * factor;
    rgb->g = (rgb->g + m) * factor;
    rgb->b = (rgb->b + m) * factor;
    rgb->a = 1.0f;

    return rgb;
}

static int HDR_RGBtoHSV(
    float r, float g, float b, float* h, float* s, float* v)
{
    float maxcol = MAX3(r, g, b);
    float mincol = MIN3(r, g, b);
    float diff = maxcol - mincol;

    HSV ret;
    //float h = -1, s = -1;
    if (maxcol == mincol) {
        *h = 0; // Use default hue.
    } else if (maxcol == r) { // Red-dominant.
        *h = fmod(60 * ((g - b) / diff) + 360, 360);
    } else if (maxcol == g) { // Green-dominant.
        *h = fmod(60 * ((b - r) / diff) + 120, 360);
    } else if (maxcol == b) { // Blue-dominant.
        *h = fmod(60 * ((r - g) / diff) + 240, 360);
    }

    if (maxcol == 0) {
        *s = 0;
    } else {
        *s = (diff / maxcol) * 100;
    }
    *v = maxcol * 100;

    return 0;
}

/*
   Grayscale.
*/

// HDR HDTV (sRGB/BT.709) standard weights.
#define GRAYSCALE_HDR_R (0.2126f)
#define GRAYSCALE_HDR_G (0.7152f)
#define GRAYSCALE_HDR_B (0.0722f)

// Standard definition (SDTV/BT.601) weights.
// This was used on image processing for CRT monitors,
// and also for PAL/NTSC format.
#define GRAYSCALE_SDR_R (0.299f)
#define GRAYSCALE_SDR_G (0.587f)
#define GRAYSCALE_SDR_B (0.114f)


#define ColorGetGrayscaleValue_Inaccurate(color) ((((color).r + (color).g + (color).b)) / 3.0f)

// More accurate estimation based on how the human eye perceives green
// as being more intense than blue.
#define ColorGetGrayscaleValue_HDR(color) ((color).r*GRAYSCALE_HDR_R + (color).g*GRAYSCALE_HDR_G + (color).b*GRAYSCALE_HDR_B)
#define ColorGetGrayscaleValue_SDR(color) ((color).r*GRAYSCALE_SDR_R + (color).g*GRAYSCALE_SDR_G + (color).b*GRAYSCALE_SDR_B)

#define ColorGetGrayscaleValue_Weights(color, weights) ((color).r*(weights)[0] + (color).g*(weights)[1] + (color).b*(weights)[2])

// Traces a line on the color cube.
// To more accurately lerp colors, you would have to lerp
// inside the HSV pyramid (or with a 3rd color in the middle).
// RGB interpolation can produce mudy intermediate colors,
// so a 3rd color can be added in the middle to balance this out.
#define ColorLerp_RGBA(out, a, b, t) do { \
    (out)->r = LERP((a)->r, (b)->r, t); \
    (out)->g = LERP((a)->g, (b)->g, t); \
    (out)->b = LERP((a)->b, (b)->b, t); \
    (out)->a = LERP((a)->a, (b)->a, t); \
} while (0)

#define ColorLerpTo_RGBA(out, target, t) do { \
    (out)->r = LERP((out)->r, (target)->r, t); \
    (out)->g = LERP((out)->g, (target)->g, t); \
    (out)->b = LERP((out)->b, (target)->b, t); \
    (out)->a = LERP((out)->a, (target)->a, t); \
} while (0)

#define ColorLerp_RGBA_Array(out, a, b, t) do { \
    (out)[0] = LERP((a)[0], (b)[0], t); \
    (out)[1] = LERP((a)[1], (b)[1], t); \
    (out)[2] = LERP((a)[2], (b)[2], t); \
    (out)[3] = LERP((a)[3], (b)[3], t); \
} while (0)



/*
   HSV interpolation is used in UI, gradients, rainbows, etc.
   Hue shifting is a digital art technique where intermediate colors
   are shifted towards the blue in shaded areas and towards yellow in lighter areas.

   It basically takes HSV interpolation one step further,
   which uses aloghrithms to dynamically push the Hue values towards
   warm or cool spectrum boundaries, depending on light intensities.
*/
extern inline COLOR* ColorLerp_HSV(COLOR* out, const COLOR* a, const COLOR* b,
    float t, float intensity)
{
    HSV interpolated, src1, src2;
    HDR_RGBtoHSV(a->r, a->g, a->b, &src1.h, &src1.s, &src1.v);
    HDR_RGBtoHSV(b->r, b->g, b->b, &src2.h, &src2.s, &src2.v);

    // Lerps by hue (color circle), then corrects for the shortest path.
    float h1 = src1.h, h2 = src2.h;
    if (fabs(h2 - h1) > 180) {
        if (h1 < h2) h1 += 360;
        else h2 += 360;
    }

    interpolated.h = LERP(h1, h2, t);
    if (interpolated.h >= 360) interpolated.h -= 360;

    // Interpolate saturation and value, too,
    // because they're linear components located on separate axes.
    interpolated.s = LERP(src1.s, src2.s, t);
    interpolated.v = LERP(src1.v, src2.v, t);

    HDR_HSVtoRGB(out, interpolated.h, interpolated.s, interpolated.v, intensity);
    out->a = LERP(a->a, b->a, t);

    return out;
}



/*
   So we define the color wheel to be in the HSV (hue-saturation-value) model.
   The hue represents the color itself.
   The colors along the radius will have the same Value,
but different Saturations, depending on the Hue and Value themselves.
   This is because of how the human eye works, when perceving different wavelengths of light,
   so green light generally has a higher saturation compared to blue light. 
*/

// Get the respective two triadic colors in a color wheel.
static inline int TrueColorTriad(COLOR* arr, const COLOR* src, float intensity)
{
    HSV hsvSrc;
    HDR_RGBtoHSV(src->r, src->g, src->b, &hsvSrc.h, &hsvSrc.s, &hsvSrc.v);

    HSV hsvTriad1 = hsvSrc;
    hsvTriad1.h = hsvSrc.h + 120.0f;
    if (hsvTriad1.h >= 360.0f) hsvTriad1.h -= 360.0f;

    HSV hsvTriad2 = hsvSrc;
    hsvTriad2.h = hsvSrc.h - 120.0f;
    if (hsvTriad2.h < 0.0f) hsvTriad2.h += 360.0f;

    COLOR* c = &arr[0];
    HDR_HSVtoRGB(c, hsvTriad1.h, hsvTriad1.s, hsvTriad1.v, intensity);
    c->a = src->a;

    c = &arr[1];
    HDR_HSVtoRGB(c, hsvTriad2.h, hsvTriad2.s, hsvTriad2.v, intensity);
    c->a = src->a;

    return 2;
}


/*
   Light color calculations (based on the Plankian black-body locus).
*/

/*
   Tanner Helland algorhithm.
   Warm colors are close to 2700K, while cool colors are close to 6000K.
*/
static COLOR* HDR_KelvinToRGB(COLOR* rgb, float kelvin, float intensity)
{
    float temp = kelvin * 0.01f;

    // Red.
    if (temp <= 66) {
        rgb->r = 255;
    } else {
        rgb->r = 329.698727446f * powf(temp - 60, -0.1332047592f);
        rgb->r = CLAMP(rgb->r, 0, 255);
    }

    // Green.
    if (temp <= 66) {
        rgb->g = 99.4708025861f * logf(temp) - 161.1195681661f;
    } else {
        rgb->g = 288.1221695283f * powf(temp - 70, -0.075514892f);
    }
    rgb->g = CLAMP(rgb->g, 0, 255);

    // Blue.
    if (temp >= 66) {
        rgb->b = 255;
    } else if (temp <= 19) {
        rgb->b = 0;
    } else {
        rgb->b = 138.5177312231f * logf(temp - 10) - 305.0447927307f;
    }
    rgb->b = CLAMP(rgb->b, 0, 255);

    float factor = (1 / 255.0f) * intensity;
    rgb->r = rgb->r * factor;
    rgb->g = rgb->g * factor;
    rgb->b = rgb->b * factor;
    rgb->a = 1.0f;

    return rgb;
}


#endif /* COLOR_H_ */
