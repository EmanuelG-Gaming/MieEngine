#include "mathf.h"

#ifdef __cplusplus
    #include "mathf_wrap.h"
#endif

#include "../base_defs.h"

#include <math.h>
#include <string.h>
#include <stdio.h>

f32 g_sin_cache[TRIG_CACHE_SIZE];
f32 g_cos_cache[TRIG_CACHE_SIZE];


void TrigCache(void)
{
    /* Trig cache size is (1 << 16) - 1. */
    for (size_t i = 0; i < TRIG_CACHE_SIZE; ++i)
    {
        f32 angle = (i / (f32) TRIG_CACHE_SIZE) * PI2;
        g_sin_cache[i] = sinf(angle);
        g_cos_cache[i] = cosf(angle);
    }
}


MATHF_IMPL COMPLEX ComplexAdd(COMPLEX a, COMPLEX b)
{
    return CLITERAL(COMPLEX) { a.re + b.re, a.im + b.im };
}
MATHF_IMPL COMPLEX ComplexSub(COMPLEX a, COMPLEX b)
{
    return CLITERAL(COMPLEX) { a.re - b.re, a.im - b.im };
}


MATHF_IMPL COMPLEX ComplexMul(COMPLEX a, COMPLEX b)
{
    /*
       (a + bi) * (c + di).
       ac + adi + cdi - ad
    */
    return CLITERAL(COMPLEX) { a.re * b.re - a.im * b.im,
                       a.re * b.im + a.im * b.re };
}


MATHF_IMPL COMPLEX ComplexDiv(COMPLEX a, COMPLEX b)
{
    complex_t len_squared = b.re*b.re + b.im*b.im;
    COMPLEX n = ComplexMul(a, ComplexConj(b));
    return ComplexScl(n, Inv(len_squared));
}

MATHF_IMPL COMPLEX ComplexConj(COMPLEX a)
{
    return CLITERAL(COMPLEX) { a.re, -a.im };
}
MATHF_IMPL COMPLEX ComplexScl(COMPLEX a, complex_t scl)
{
    return CLITERAL(COMPLEX) { a.re * scl, a.im * scl };
}

/* e^ia. */
MATHF_IMPL COMPLEX ComplexExp(complex_t angle)
{
    return CLITERAL(COMPLEX) { Cos(angle), Sin(angle) };
}


MATHF_IMPL complex_t ComplexLength(COMPLEX a)
{
    return Sqrt(ComplexLength2(a));
}
MATHF_IMPL complex_t ComplexLength2(COMPLEX a)
{
    return (a.re * a.re + a.im * a.im);
}

MATHF_IMPL complex_t ComplexDst(COMPLEX a, COMPLEX b)
{
    complex_t dx = b.re - a.re;
    complex_t dy = b.im - a.im;

    return Sqrt(dx * dx + dy * dy);
}
MATHF_IMPL complex_t ComplexDst2(COMPLEX a, COMPLEX b)
{
    complex_t dx = b.re - a.re;
    complex_t dy = b.im - a.im;

    return (dx * dx + dy * dy);
}


MATHF_IMPL int IsPower2(size_t n)
{
    return ((n > 0) && (n & (n - 1)) == 0);
}

/* The logarithm. */
/* TODO: MATHF_IMPL function when? */

#ifdef _MSC_VER

static __inline uint32_t clz(uint32_t x)
{
    DWORD leading_zero = 0;

    if (_BitScanReverse(&leading_zero, x))
    {
        return 31 - leading_zero;
    }
    else {
        return 32;
    }
}

#else

static __inline uint32_t clz(uint32_t x)
{
    return __builtin_clz(x);
}
#endif


MATHF_IMPL i32 GetLog2(size_t n)
{
    if (n == 0) return -1; /* log(0) is undefined. */
    // GCC/clang.
    return (sizeof(unsigned int) * 8 - 1) - clz(n);
}

#define FFT_MAX 1028
/* Radix-2 FFT algorithm (Cooley-Tukey method). */
void FFT(COMPLEX* x, size_t count)
{
    COMPLEX even[FFT_MAX/2], odd[FFT_MAX/2], twiddle;
    size_t i, j, half_size;

    size_t p2 = GetLog2(count); /* Assume that count is a pow of 2. */
  
    /* Bit reversal permutation. */
    for (i = 0; i < count; ++i)
    {
        int reversed = 0;
        for (j = 0; j < p2; ++j)
        {
            reversed = (reversed << 1) | (i >> j & 1);  

            if (reversed > i) {
                COMPLEX temp = x[i]; 
                x[i] = x[reversed];
                x[reversed] = temp;
            }
        }
    }
  
    /* Danielson-Lanczos Algorithm. */
    for (half_size = 1; half_size < count; half_size <<= 1)
    { /* *=2 */
        for (i = 0; i < count; i += 2*half_size)
        {
            for (j = 0; j < half_size; ++j)
            {
                even[j] = x[i + j];
                odd[j]  = x[i + j + half_size];

                double angle = -PI2 * j / (2*half_size);
                twiddle = ComplexExp(angle);

                /* The butterfly effect. */
                x[i + j]            = ComplexAdd(even[j], ComplexMul(twiddle, odd[j]));
                x[i + j +half_size] = ComplexSub(even[j], ComplexMul(twiddle, odd[j]));
            }
        }
    }
}


/*
   Vector operations.
*/

/*
   2-dimensional vectors.
   I didn't figure out how to do SSE on 2 floats,
   mostly because SSE is 128-bit registers and I am using 64 bits
   to store vec2s.
*/

extern MATHF_IMPL void Vec2Zero(vec2* out)
{
    out->x = out->y = 0;
}
extern MATHF_IMPL void Vec2Copy(vec2* out, const vec2* v)
{
    out->x = v->x;
    out->y = v->y;
}
extern MATHF_IMPL vec2* Vec2Add(vec2* out, const vec2* a, const vec2* b)
{
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    return out;
}
extern MATHF_IMPL vec2* Vec2Sub(vec2* out, const vec2* a, const vec2* b)
{
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    return out;
}
extern MATHF_IMPL vec2* Vec2Mul(vec2* out, const vec2* a, const vec2* b)
{
    out->x = a->x * b->x;
    out->y = a->y * b->y;
    return out;
}
extern MATHF_IMPL vec2* Vec2Div(vec2* out, const vec2* a, const vec2* b)
{
    out->x = a->x / b->x;
    out->y = a->y / b->y;
    return out;
}
extern MATHF_IMPL vec2* Vec2AddScalar(vec2* out, const vec2* a, float s)
{
    out->x = a->x + s;
    out->y = a->y + s;
    return out;
}
extern MATHF_IMPL vec2* Vec2SubScalar(vec2* out, const vec2* a, float s)
{
    out->x = a->x - s;
    out->y = a->y - s;
    return out;
}
extern MATHF_IMPL vec2* Vec2MulScalar(vec2* out, const vec2* a, float s)
{
    out->x = a->x * s;
    out->y = a->y * s;
    return out;
}
extern MATHF_IMPL vec2* Vec2DivScalar(vec2* out, const vec2* a, float s)
{
    out->x = a->x / s;
    out->y = a->y / s;
    return out;
}
extern vec2* Vec2Lerp(vec2* out, const vec2* a, const vec2* b, float t)
{
    out->x = LERP(a->x, b->x, t);
    out->y = LERP(a->y, b->y, t);
    return out;
}



#if MATHF_USE_SSE

/*
   3-dimensional vectors.
*/


MATHF_IMPL void Vec3Zero(vec3* out)
{
    _mm_store_ps(&out->x, _mm_set_ps1(0));
}
MATHF_IMPL void Vec3Copy(vec3* out, const vec3* v)
{
    _mm_store_ps(&out->x, _mm_load_ps(&v->x));
}
MATHF_IMPL vec3* Vec3Add(vec3* out, const vec3* a, const vec3* b)
{
    __m128 v = _mm_add_ps(_mm_load_ps(&a->x), _mm_load_ps(&b->x));
    _mm_store_ps(&out->x, v);
    return out;
}
MATHF_IMPL vec3* Vec3Sub(vec3* out, const vec3* a, const vec3* b)
{
    __m128 v = _mm_sub_ps(_mm_load_ps(&a->x), _mm_load_ps(&b->x));
    _mm_store_ps(&out->x, v);
    return out;
}
MATHF_IMPL vec3* Vec3Mul(vec3* out, const vec3* a, const vec3* b)
{
    __m128 v = _mm_mul_ps(_mm_load_ps(&a->x), _mm_load_ps(&b->x));
    _mm_store_ps(&out->x, v);
    return out;
}
MATHF_IMPL vec3* Vec3Div(vec3* out, const vec3* a, const vec3* b)
{
    __m128 v = _mm_div_ps(_mm_load_ps(&a->x), _mm_load_ps(&b->x));
    _mm_store_ps(&out->x, v);
    return out;
}
MATHF_IMPL vec3* Vec3AddScalar(vec3* out, const vec3* a, float s)
{
    __m128 v = _mm_add_ps(_mm_load_ps(&a->x), _mm_set_ps1(s));
    _mm_store_ps(&out->x, v);
    return out;
}
MATHF_IMPL vec3* Vec3SubScalar(vec3* out, const vec3* a, float s)
{
    __m128 v = _mm_sub_ps(_mm_load_ps(&a->x), _mm_set_ps1(s));
    _mm_store_ps(&out->x, v);
    return out;
}
MATHF_IMPL vec3* Vec3MulScalar(vec3* out, const vec3* a, float s)
{
    __m128 v = _mm_mul_ps(_mm_load_ps(&a->x), _mm_set_ps1(s));
    _mm_store_ps(&out->x, v);
    return out;
}
MATHF_IMPL vec3* Vec3DivScalar(vec3* out, const vec3* a, float s)
{
    __m128 v = _mm_div_ps(_mm_load_ps(&a->x), _mm_set_ps1(s));
    _mm_store_ps(&out->x, v);
    return out;
}

/* a * (1-t) + b*t. */
MATHF_IMPL vec3* Vec3Lerp(vec3* out, const vec3* a, const vec3* b, float t)
{
    __m128 t2 = splat_x(_mm_set_ss(t));
    __m128 t3 = _mm_sub_ps(_mm_set_ps1(1.0f), t2);

    __m128 a2 = _mm_mul_ps(t3, _mm_load_ps(&a->x));
    __m128 b2 = _mm_mul_ps(t2, _mm_load_ps(&b->x));
    _mm_store_ps(&out->x, _mm_add_ps(a2, b2));
    return out;
}

/*
   4-dimensional vectors.
*/

MATHF_IMPL void Vec4Zero(vec4* out)
{
    _mm_store_ps(&out->x, _mm_set_ps1(0));
}
MATHF_IMPL void Vec4Copy(vec4* out, const vec4* v)
{
    _mm_store_ps(&out->x, _mm_load_ps(&v->x));
}
MATHF_IMPL vec4* Vec4Add(vec4* out, const vec4* a, const vec4* b)
{
    __m128 v = _mm_add_ps(_mm_load_ps(&a->x), _mm_load_ps(&b->x));
    _mm_store_ps(&out->x, v);
    return out;
}
MATHF_IMPL vec4* Vec4Sub(vec4* out, const vec4* a, const vec4* b)
{
    __m128 v = _mm_sub_ps(_mm_load_ps(&a->x), _mm_load_ps(&b->x));
    _mm_store_ps(&out->x, v);
    return out;
}
MATHF_IMPL vec4* Vec4Mul(vec4* out, const vec4* a, const vec4* b)
{
    __m128 v = _mm_mul_ps(_mm_load_ps(&a->x), _mm_load_ps(&b->x));
    _mm_store_ps(&out->x, v);
    return out;
}
MATHF_IMPL vec4* Vec4Div(vec4* out, const vec4* a, const vec4* b)
{
    __m128 v = _mm_div_ps(_mm_load_ps(&a->x), _mm_load_ps(&b->x));
    _mm_store_ps(&out->x, v);
    return out;
}
MATHF_IMPL vec4* Vec4AddScalar(vec4* out, const vec4* a, float s)
{
    __m128 v = _mm_add_ps(_mm_load_ps(&a->x), _mm_set_ps1(s));
    _mm_store_ps(&out->x, v);
    return out;
}
MATHF_IMPL vec4* Vec4SubScalar(vec4* out, const vec4* a, float s)
{
    __m128 v = _mm_sub_ps(_mm_load_ps(&a->x), _mm_set_ps1(s));
    _mm_store_ps(&out->x, v);
    return out;
}
MATHF_IMPL vec4* Vec4MulScalar(vec4* out, const vec4* a, float s)
{
    __m128 v = _mm_mul_ps(_mm_load_ps(&a->x), _mm_set_ps1(s));
    _mm_store_ps(&out->x, v);
    return out;
}

MATHF_IMPL vec4* Vec4DivScalar(vec4* out, const vec4* a, float s)
{
    __m128 v = _mm_div_ps(_mm_load_ps(&a->x), _mm_set_ps1(s));
    _mm_store_ps(&out->x, v);
    return out;
}

/* a * (1-t) + b*t. */
MATHF_IMPL vec4* Vec4Lerp(vec4* out, const vec4* a, const vec4*b, float t)
{
    __m128 t2 = splat_x(_mm_set_ss(t));
    __m128 t3 = _mm_sub_ps(_mm_set_ps1(1.0f), t2);

    __m128 a2 = _mm_mul_ps(t3, _mm_load_ps(&a->x));
    __m128 b2 = _mm_mul_ps(t2, _mm_load_ps(&b->x));
    _mm_store_ps(&out->x, _mm_add_ps(a2, b2));
    return out;
}

extern MATHF_IMPL vec4* Vec4FromTrPos(vec4* out, const TRANSFORM* tr)
{
    _mm_store_ps(&out->x, _mm_loadu_ps(&tr->x));
    return out;
}

extern MATHF_IMPL vec4* Vec4FromTrRot(vec4* out, const TRANSFORM* tr)
{
    _mm_store_ps(&out->x, _mm_loadu_ps(&tr->rx));
    return out;
}

extern MATHF_IMPL vec4* Vec4FromTrScl(vec4* out, const TRANSFORM* tr)
{
    _mm_store_ps(&out->x, _mm_loadu_ps(&tr->sx));
    return out;
}

/*
   Matrix operations.
*/

MATHF_IMPL mat3* Mat3Copy(mat3* out, const mat3* a)
{
    _mm_store_ps(&out->c[0], _mm_load_ps(&a->c[0]));
    _mm_store_ps(&out->c[3], _mm_load_ps(&a->c[3]));
    _mm_store_ps(&out->c[6], _mm_load_ps(&a->c[6]));
    return out;
}

MATHF_IMPL mat3* Mat3Mul(mat3 *out, const mat3 *a, const mat3 *b)
{
    /* TODO: Maybe causes some issues. */

    // Temporary SIMD registers
    __m128 l, r0, r1, r2,
              v0, v1, v2;

    r0 = _mm_load_ps(&b->c[0]);
    r1 = _mm_load_ps(&b->c[3]);
    r2 = _mm_load_ps(&b->c[6]);

    l = _mm_load_ps(&a->c[0]);
    v0 = _mm_mul_ps(l, splat_x(r0));
    v1 = _mm_mul_ps(l, splat_x(r1));
    v2 = _mm_mul_ps(l, splat_x(r2));

    l = _mm_load_ps(&a->c[3]);
    v0 = _mm_add_ps(v0, _mm_mul_ps(l, splat_y(r0)));
    v1 = _mm_add_ps(v1, _mm_mul_ps(l, splat_y(r1)));
    v2 = _mm_add_ps(v2, _mm_mul_ps(l, splat_y(r2)));

    l = _mm_load_ps(&a->c[6]);
    v0 = _mm_add_ps(v0, _mm_mul_ps(l, splat_z(r0)));
    v1 = _mm_add_ps(v1, _mm_mul_ps(l, splat_z(r1)));
    v2 = _mm_add_ps(v2, _mm_mul_ps(l, splat_z(r2)));


    _mm_store_ps(&out->c[0], v0);
    _mm_store_ps(&out->c[3], v1);
    _mm_store_ps(&out->c[6], v2);
    return out;
}

MATHF_IMPL vec3* Mat3MulVec3(vec3* out, const mat3* a, const vec3* b)
{
    __m128 r = _mm_loadu_ps(&b->x);

    __m128 l0 = _mm_load_ps(&a->c[0]); 
    __m128 l1 = _mm_load_ps(&a->c[3]); 
    __m128 l2 = _mm_load_ps(&a->c[6]); 

    __m128 v0 = _mm_mul_ps(l0, splat_x(r));
    v0 = _mm_add_ps(v0, _mm_mul_ps(l1, splat_y(r)));
    v0 = _mm_add_ps(v0, _mm_mul_ps(l2, splat_z(r)));

    _mm_storeu_ps(&out->x, v0);
    return out;
}

/*
   4-dimensional matrices.
*/

MATHF_IMPL mat4* Mat4Copy(mat4* out, const mat4* a)
{
    _mm_store_ps(&out->c[0], _mm_load_ps(&a->c[0]));
    _mm_store_ps(&out->c[4], _mm_load_ps(&a->c[4]));
    _mm_store_ps(&out->c[8], _mm_load_ps(&a->c[8]));
    _mm_store_ps(&out->c[12], _mm_load_ps(&a->c[12]));
    return out;
}

MATHF_IMPL mat4* Mat4Mul(mat4 *out, const mat4 *a, const mat4 *b)
{
    // Temporary SIMD registers
    __m128 l, r0, r1, r2, r3,
              v0, v1, v2, v3;

    r0 = _mm_load_ps(&b->c[0]);
    r1 = _mm_load_ps(&b->c[4]);
    r2 = _mm_load_ps(&b->c[8]);
    r3 = _mm_load_ps(&b->c[12]);

    l = _mm_load_ps(&a->c[0]);
    v0 = _mm_mul_ps(l, splat_x(r0));
    v1 = _mm_mul_ps(l, splat_x(r1));
    v2 = _mm_mul_ps(l, splat_x(r2));
    v3 = _mm_mul_ps(l, splat_x(r3));

    l = _mm_load_ps(&a->c[4]);
    v0 = _mm_add_ps(v0, _mm_mul_ps(l, splat_y(r0)));
    v1 = _mm_add_ps(v1, _mm_mul_ps(l, splat_y(r1)));
    v2 = _mm_add_ps(v2, _mm_mul_ps(l, splat_y(r2)));
    v3 = _mm_add_ps(v3, _mm_mul_ps(l, splat_y(r3)));

    l = _mm_load_ps(&a->c[8]);
    v0 = _mm_add_ps(v0, _mm_mul_ps(l, splat_z(r0)));
    v1 = _mm_add_ps(v1, _mm_mul_ps(l, splat_z(r1)));
    v2 = _mm_add_ps(v2, _mm_mul_ps(l, splat_z(r2)));
    v3 = _mm_add_ps(v3, _mm_mul_ps(l, splat_z(r3)));

    l = _mm_load_ps(&a->c[12]);
    v0 = _mm_add_ps(v0, _mm_mul_ps(l, splat_w(r0)));
    v1 = _mm_add_ps(v1, _mm_mul_ps(l, splat_w(r1)));
    v2 = _mm_add_ps(v2, _mm_mul_ps(l, splat_w(r2)));
    v3 = _mm_add_ps(v3, _mm_mul_ps(l, splat_w(r3)));

    _mm_store_ps(&out->c[0], v0);
    _mm_store_ps(&out->c[4], v1);
    _mm_store_ps(&out->c[8], v2);
    _mm_store_ps(&out->c[12], v3);
    return out;
}

MATHF_IMPL vec4* Mat4MulVec4(vec4* out, const mat4* a, const vec4* b)
{
    __m128 r = _mm_loadu_ps(&b->x);

    __m128 l0 = _mm_load_ps(&a->c[0]); 
    __m128 l1 = _mm_load_ps(&a->c[4]); 
    __m128 l2 = _mm_load_ps(&a->c[8]); 
    __m128 l3 = _mm_load_ps(&a->c[12]); 

    __m128 v0 = _mm_mul_ps(l0, splat_x(r));
    v0 = _mm_add_ps(v0, _mm_mul_ps(l1, splat_y(r)));
    v0 = _mm_add_ps(v0, _mm_mul_ps(l2, splat_z(r)));
    v0 = _mm_add_ps(v0, _mm_mul_ps(l3, splat_w(r)));

    _mm_storeu_ps(&out->x, v0);
    return out;
}

#else

/*
   3-dimensional vectors.
*/

MATHF_IMPL void Vec3Zero(vec3* out)
{
    out->x = out->y = out->z = 0;
}
MATHF_IMPL void Vec3Copy(vec3* out, const vec3* v)
{
    out->x = v->x;
    out->y = v->y;
    out->z = v->z;
}
MATHF_IMPL vec3* Vec3Add(vec3* out, const vec3* a, const vec3* b)
{
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    out->z = a->z + b->z;
    return out;
}

MATHF_IMPL vec3* Vec3Sub(vec3* out, const vec3* a, const vec3* b)
{
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    out->z = a->z - b->z;
    return out;
}
MATHF_IMPL vec3* Vec3Mul(vec3* out, const vec3* a, const vec3* b)
{
    out->x = a->x * b->x;
    out->y = a->y * b->y;
    out->z = a->z * b->z;
    return out;
}

MATHF_IMPL vec3* Vec3Div(vec3* out, const vec3* a, const vec3* b)
{
    out->x = a->x / b->x;
    out->y = a->y / b->y;
    out->z = a->z / b->z;
    return out;
}
MATHF_IMPL vec3* Vec3AddScalar(vec3* out, const vec3* a, float s)
{
    out->x = a->x + s;
    out->y = a->y + s;
    out->z = a->z + s;
    return out;
}
MATHF_IMPL vec3* Vec3SubScalar(vec3* out, const vec3* a, float s)
{
    out->x = a->x - s;
    out->y = a->y - s;
    out->z = a->z - s;
    return out;
}
MATHF_IMPL vec3* Vec3MulScalar(vec3* out, const vec3* a, float s)
{
    out->x = a->x * s;
    out->y = a->y * s;
    out->z = a->z * s;
    return out;
}
MATHF_IMPL vec3* Vec3DivScalar(vec3* out, const vec3* a, float s)
{
    out->x = a->x / s;
    out->y = a->y / s;
    out->z = a->z / s;
    return out;
}

/* a * (1-t) + b*t. */
MATHF_IMPL vec3* Vec3Lerp(vec3* out, const vec3* a, const vec3* b, float t)
{
    out->x = LERP(a->x, b->x, t);
    out->y = LERP(a->y, b->y, t);
    out->z = LERP(a->z, b->z, t);
    return out;
}

/*
   4-dimensional vectors.
*/

MATHF_IMPL void Vec4Zero(vec4* out)
{
    out->x = out->y = out->z = out->w = 0;
}
MATHF_IMPL void Vec4Copy(vec4* out, const vec4* v)
{
    out->x = v->x;
    out->y = v->y;
    out->z = v->z;
    out->w = v->w;
}
MATHF_IMPL vec4* Vec4Add(vec4* out, const vec4* a, const vec4* b)
{
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    out->z = a->z + b->z;
    out->w = a->w + b->w;
    return out;
}

MATHF_IMPL vec4* Vec4Sub(vec4* out, const vec4* a, const vec4* b)
{
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    out->z = a->z - b->z;
    out->w = a->w - b->w;
    return out;
}
MATHF_IMPL vec4* Vec4Mul(vec4* out, const vec4* a, const vec4* b)
{
    out->x = a->x * b->x;
    out->y = a->y * b->y;
    out->z = a->z * b->z;
    out->w = a->w * b->w;
    return out;
}

MATHF_IMPL vec4* Vec4Div(vec4* out, const vec4* a, const vec4* b)
{
    out->x = a->x / b->x;
    out->y = a->y / b->y;
    out->z = a->z / b->z;
    out->w = a->w / b->w;
    return out;
}
MATHF_IMPL vec4* Vec4AddScalar(vec4* out, const vec4* a, float s)
{
    out->x = a->x + s;
    out->y = a->y + s;
    out->z = a->z + s;
    out->w = a->w + s;
    return out;
}
MATHF_IMPL vec4* Vec4SubScalar(vec4* out, const vec4* a, float s)
{
    out->x = a->x - s;
    out->y = a->y - s;
    out->z = a->z - s;
    out->w = a->w - s;
    return out;
}
MATHF_IMPL vec4* Vec4MulScalar(vec4* out, const vec4* a, float s)
{
    out->x = a->x * s;
    out->y = a->y * s;
    out->z = a->z * s;
    out->w = a->w * s;
    return out;
}
MATHF_IMPL vec4* Vec4DivScalar(vec4* out, const vec4* a, float s)
{
    out->x = a->x / s;
    out->y = a->y / s;
    out->z = a->z / s;
    out->w = a->w / s;
    return out;
}

/* a * (1-t) + b*t. */
MATHF_IMPL vec4* Vec4Lerp(vec4* out, const vec4* a, const vec4* b, float t)
{
    out->x = LERP(a->x, b->x, t);
    out->y = LERP(a->y, b->y, t);
    out->z = LERP(a->z, b->z, t);
    out->w = LERP(a->w, b->w, t);
    return out;
}


/*
   Matrix operations.
*/


MATHF_IMPL mat3* Mat3Copy(mat3* out, const mat3* a)
{
    for (size_t i = 0; i < 9; ++i) {
        out->c[i] = a->c[i];
    }
    return out;
}

MATHF_IMPL mat3* Mat3Mul(mat3 *out, const mat3 *a, const mat3 *b)
{
    size_t i, j, k;
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {

            out->c[i*3+j] = 0;
            for (k = 0; k < 3; ++k) {
                out->c[i*3+j] += a->c[i*3+k] * b->c[k*3+j];
            }
        }
    }

    return out;
}

MATHF_IMPL vec3* Mat3MulVec3(vec3* out, const mat3* a, const vec3* b)
{
    size_t i, j;
    for (i = 0; i < 3; ++i) {
        out->ve[i] = 0;
        for (j = 0; j < 3; ++j) {
            out->ve[i] += a->c[i*3+j] * b->ve[j];
        }
    }

    return out;
}



MATHF_IMPL mat4* Mat4Copy(mat4* out, const mat4* a)
{
    for (size_t i = 0; i < 16; ++i) {
        out->c[i] = a->c[i];
    }
    return out;
}

MATHF_IMPL mat4* Mat4Mul(mat4 *out, const mat4 *a, const mat4 *b)
{
    size_t i, j, k;
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {

            out->c[i*4+j] = 0;
            for (k = 0; k < 4; ++k) {
                out->c[i*4+j] += a->c[i*4+k] * b->c[k*4+j];
            }
        }
    }

    return out;
}

MATHF_IMPL vec4* Mat4MulVec4(vec4* out, const mat4* a, const vec4* b)
{
    size_t i, j;
    for (i = 0; i < 4; ++i) {
        out->v[i] = 0;
        for (j = 0; j < 4; ++j) {
            out->v[i] += a->c[i*4+j] * b->v[j];
        }
    }

    return out;
}

#endif


MATHF_IMPL vec4* Mat4MulVec3(vec4* out, const mat4* a, vec4* b)
{
    b->w = 1.0f;
    return Mat4MulVec4(out, a, b);
}

/* :%s/<str to find>/<replacement>/g */

//#error "No SSE found for compatibility"



/*
   Front-end operations.
*/

/*
   Vectors.
*/

/*
   2-dimensional vectors.
*/

MATHF_IMPL vec2* Vec2Set2(vec2* out, float x, float y)
{
    out->x = x;
    out->y = y;
    return out;
}
MATHF_IMPL vec2* Vec2Set1(vec2* out, float x)
{
    out->x = x;
    return out;
}
MATHF_IMPL vec2* Vec2SetV2(vec2* out, const vec2* src)
{
    out->x = src->x;
    out->y = src->y;
    return out;
}
MATHF_IMPL vec3* Vec2SetV1(vec3* out, const vec3* src)
{
    out->x = src->x;
    return out;
}


MATHF_IMPL float Vec2Dot2(const vec2* a, const vec2* b)
{
    return a->x*b->x + a->y*b->y;
}
MATHF_IMPL float Vec2Len2(const vec2* v)
{
    return sqrtf(v->x*v->x + v->y*v->y);
}
MATHF_IMPL float Vec2Len2Squared(const vec2* v)
{
    return v->x*v->x + v->y*v->y; 
}
MATHF_IMPL float Vec2Dst2(const vec2* v1, const vec2* v2)
{
    vec2 d; Vec2Sub(&d, v1, v2);
    return Vec2Len2(&d);
}

MATHF_IMPL float Vec2Dst2Squared(const vec2* v1, const vec2* v2)
{
    vec2 d; Vec2Sub(&d, v1, v2);
    return Vec2Len2Squared(&d);
}
MATHF_IMPL vec2* Vec2Nor2(vec2* out, const vec2* v)
{
    return Vec2MulScalar(out, v, 1 / Vec2Len2(v));
}

MATHF_IMPL vec2* Vec2Round2(vec2* out, const vec2* v)
{
    out->x = roundf(v->x);
    out->y = roundf(v->y);
    return out;
}

MATHF_IMPL float Vec2Cross(const vec2* a, const vec2* b)
{
    // Determinant of two vectors.
    // | x1 x2 |
    // | y1 y2 |
    return (a->x*b->y - b->x*a->y);
}

MATHF_IMPL float Vec2CrossTri(const vec2 *a, const vec2 *b, const vec2 *c)
{
    vec2 ab; Vec2Sub(&ab, b, a);
    vec2 ac; Vec2Sub(&ac, c, a);

    return Vec2Cross(&ab, &ac);
}

/*
   3-dimensional vectors.
*/


MATHF_IMPL vec3* Vec3Set3(vec3* out, float x, float y, float z)
{
    out->x = x;
    out->y = y;
    out->z = z;
    return out;
}
MATHF_IMPL vec3* Vec3Set2(vec3* out, float x, float y)
{
    return Vec3Set3(out, x, y, 1);
}
MATHF_IMPL vec3* Vec3Set1(vec3* out, float x)
{
    return Vec3Set3(out, x, 1, 1);
}

MATHF_IMPL vec3* Vec3SetV4(vec3* out, const vec4* src)
{
    return Vec3Set3(out, src->x, src->y, src->z);
}
MATHF_IMPL vec3* Vec3SetV3(vec3* out, const vec3* src)
{
    return Vec3Set3(out, src->x, src->y, src->z);
}
MATHF_IMPL vec3* Vec3SetV2(vec3* out, const vec3* src)
{
    return Vec3Set2(out, src->x, src->y);
}
MATHF_IMPL vec3* Vec3SetV1(vec3* out, const vec3* src)
{
    return Vec3Set1(out, src->x);
}


MATHF_IMPL float Vec3Dot3(const vec3* a, const vec3* b)
{
    return a->x*b->x + a->y*b->y + a->z*b->z;
}
MATHF_IMPL float Vec3Len3(const vec3* v)
{
    return sqrtf(v->x*v->x + v->y*v->y + v->z*v->z);
}
MATHF_IMPL float Vec3Len3Squared(const vec3* v)
{
    return v->x*v->x + v->y*v->y + v->z*v->z; 
}
MATHF_IMPL float Vec3Dst3(const vec3* v1, const vec3* v2)
{
    vec3 d; Vec3Sub(&d, v1, v2);
    return Vec3Len3(&d);
}

MATHF_IMPL float Vec3Dst3Squared(const vec3* v1, const vec3* v2)
{
    vec3 d; Vec3Sub(&d, v1, v2);
    return Vec3Len3Squared(&d);
}
MATHF_IMPL vec3* Vec3Nor3(vec3* out, const vec3* v)
{
    return Vec3MulScalar(out, v, 1 / Vec3Len3(v));
    //return Vec3MulScalar(out, v, 2);
}

MATHF_IMPL vec3* Vec3Round3(vec3* out, const vec3* v)
{
    out->x = roundf(v->x);
    out->y = roundf(v->y);
    out->z = roundf(v->z);
    return out;
}

MATHF_IMPL vec3* Vec3Cross3(vec3* out, const vec3* a, const vec3* b)
{
    out->x = a->y * b->z - a->z * b->y;
	out->y = a->z * b->x - a->x * b->z;
	out->z = a->x * b->y - a->y * b->x;
	return out;
}

MATHF_IMPL vec3* Vec3RotateAxisAngle3(vec3* v, const vec3* axis, float angle)
{
    vec3 nor_axis; Vec3Nor3(&nor_axis, axis);

    angle /= 2.0f;
    float a = sinf(angle);
    float b = nor_axis.x*a;
    float c = nor_axis.y*a;
    float d = nor_axis.z*a;
    a = cosf(angle);

    /* TODO: Excess elements in union initializer. */
    vec3 w = CLITERAL(vec3) { b, c, d };

    vec3 wv; Vec3Cross3(&wv, &w, v);

    vec3 wwv; Vec3Cross3(&wwv, &w, &wv);

	Vec3MulScalar(&wwv, &wv, 2*a);
	Vec3MulScalar(&wwv, &wwv, 2);

	// Then add them together
	Vec3Add(v, v, &wv);
	Vec3Add(v, v, &wwv);
    return v;
}

MATHF_IMPL float Vec3Angle3(const vec3* v1, const vec3* v2)
{
    float result = 0.0f;

	vec3 cross; Vec3Cross3(&cross, v1, v2);
	float len = Vec3Len3(&cross);
	float dot = Vec3Dot3(v1, v2);
    result = atan2f(len, dot);

    return result;
}


extern MATHF_IMPL vec3* Vec3Outer(vec3* out, const vec3* a, const vec3* b)
{
    out->x = a->y * b->z - a->z * b->y;
	out->y = a->z * b->x - a->x * b->z;
	out->z = a->x * b->y - a->y * b->x;
	return out;
}

extern MATHF_IMPL float Vec3Inner(const vec3* v1, const vec3* v2)
{
    return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

extern MATHF_IMPL vec3* Vec3Clamp(vec3* out, const vec3* a, const vec3* min, const vec3* max)
{
    out->x = CLAMP(a->x, min->x, max->x);
    out->y = CLAMP(a->y, min->y, max->y);
    out->z = CLAMP(a->z, min->z, max->z);
    return out;
}
extern MATHF_IMPL vec3* Vec3Min(vec3* out, const vec3* a, const vec3* b)
{
    out->x = MIN(a->x, b->x);
    out->y = MIN(a->y, b->y);
    out->z = MIN(a->z, b->z);
    return out;
}
extern MATHF_IMPL vec3* Vec3Max(vec3* out, const vec3* a, const vec3* b)
{
    out->x = MAX(a->x, b->x);
    out->y = MAX(a->y, b->y);
    out->z = MAX(a->z, b->z);
    return out;
}


/*
   4-dimensional vectors.
*/

MATHF_IMPL vec4* Vec4Set4(vec4* out, float x, float y, float z, float w)
{
    out->x = x;
    out->y = y;
    out->z = z;
    out->w = w;
    return out;
}
MATHF_IMPL vec4* Vec4Set3(vec4* out, float x, float y, float z)
{
    return Vec4Set4(out, x, y, z, 1);
}
MATHF_IMPL vec4* Vec4Set2(vec4* out, float x, float y)
{
    return Vec4Set4(out, x, y, 1, 1);
}
MATHF_IMPL vec4* Vec4Set1(vec4* out, float x)
{
    return Vec4Set4(out, x, 1, 1, 1);
}

MATHF_IMPL vec4* Vec4SetV4(vec4* out, const vec4* src)
{
    return Vec4Set4(out, src->x, src->y, src->z, src->w);
}
MATHF_IMPL vec4* Vec4SetV3(vec4* out, const vec4* src)
{
    return Vec4Set3(out, src->x, src->y, src->z);
}
MATHF_IMPL vec4* Vec4SetV2(vec4* out, const vec4* src)
{
    return Vec4Set2(out, src->x, src->y);
}
MATHF_IMPL vec4* Vec4SetV1(vec4* out, const vec4* src)
{
    return Vec4Set1(out, src->x);
}


MATHF_IMPL float Vec4Dot3(const vec4* a, const vec4* b)
{
    return a->x*b->x + a->y*b->y + a->z*b->z;
}
MATHF_IMPL float Vec4Len3(const vec4* v)
{
    return sqrtf(v->x*v->x + v->y*v->y + v->z*v->z);
}
MATHF_IMPL float Vec4Len3Squared(const vec4* v)
{
    return v->x*v->x + v->y*v->y + v->z*v->z; 
}
MATHF_IMPL float Vec4Dst3(const vec4* v1, const vec4* v2)
{
    vec4 d; Vec4Sub(&d, v1, v2);
    return Vec4Len3(&d);
}

MATHF_IMPL float Vec4Dst3Squared(const vec4* v1, const vec4* v2)
{
    vec4 d; Vec4Sub(&d, v1, v2);
    return Vec4Len3Squared(&d);
}
MATHF_IMPL vec4* Vec4Nor3(vec4* out, const vec4* v)
{
    return Vec4MulScalar(out, v, 1 / Vec4Len3(v));
}


MATHF_IMPL float Vec4Dot4(const vec4* a, const vec4* b)
{
    return a->x*b->x + a->y*b->y + a->z*b->z + a->w*b->w;
}
MATHF_IMPL float Vec4Len4(const vec4* v)
{
    return sqrtf(v->x*v->x + v->y*v->y + v->z*v->z + v->w*v->w);
}
MATHF_IMPL float Vec4Len4Squared(const vec4* v)
{
    return (v->x*v->x + v->y*v->y + v->z*v->z + v->w*v->w);
}

MATHF_IMPL float Vec4Dst4(const vec4* v1, const vec4* v2)
{
    vec4 d; Vec4Sub(&d, v1, v2);
    return Vec4Len4(&d);
}

MATHF_IMPL float Vec4Dst4Squared(const vec4* v1, const vec4* v2)
{
    vec4 d; Vec4Sub(&d, v1, v2);
    return Vec4Len4Squared(&d);
}
MATHF_IMPL vec4* Vec4Nor4(vec4* out, const vec4* v)
{
    return Vec4MulScalar(out, v, 1 / Vec4Len4(v));
}



MATHF_IMPL vec4* Vec4Round3(vec4* out, const vec4* v)
{
    out->x = roundf(v->x);
    out->y = roundf(v->y);
    out->z = roundf(v->z);
    return out;
}
MATHF_IMPL vec4* Vec4Round4(vec4* out, const vec4* v)
{
    out->x = roundf(v->x);
    out->y = roundf(v->y);
    out->z = roundf(v->z);
    out->w = roundf(v->w);
    return out;
}

MATHF_IMPL vec4* Vec4Cross3(vec4* out, const vec4* a, const vec4* b)
{
    out->x = a->y * b->z - a->z * b->y;
	out->y = a->z * b->x - a->x * b->z;
	out->z = a->x * b->y - a->y * b->x;
	return out;
}

MATHF_IMPL vec4* Vec4RotateAxisAngle3(vec4* v, const vec4* axis, float angle)
{
    vec4 nor_axis; Vec4Nor3(&nor_axis, axis);

    angle /= 2.0f;
    float a = sinf(angle);
    float b = nor_axis.x*a;
    float c = nor_axis.y*a;
    float d = nor_axis.z*a;
    a = cosf(angle);
    vec4 w = CLITERAL(vec4) { b, c, d, 0 };

    vec4 wv; Vec4Cross3(&wv, &w, v);

    vec4 wwv; Vec4Cross3(&wwv, &w, &wv);

	Vec4MulScalar(&wwv, &wv, 2*a);
	Vec4MulScalar(&wwv, &wwv, 2);

	// Then add them together
	Vec4Add(v, v, &wv);
	Vec4Add(v, v, &wwv);
    return v;
}

MATHF_IMPL float Vec4Angle3(const vec4* v1, const vec4* v2)
{
    float result = 0.0f;

	vec4 cross; Vec4Cross3(&cross, v1, v2);
	float len = Vec4Len3(&cross);
	float dot = Vec4Dot3(v1, v2);
    result = atan2f(len, dot);

    return result;
}

extern MATHF_IMPL vec4* Vec4Clamp(vec4* out, const vec4* a, const vec4* min, const vec4* max)
{
    out->x = CLAMP(a->x, min->x, max->x);
    out->y = CLAMP(a->y, min->y, max->y);
    out->z = CLAMP(a->z, min->z, max->z);
    out->w = CLAMP(a->w, min->w, max->w);
    return out;
}
extern MATHF_IMPL vec4* Vec4Min(vec4* out, const vec4* a, const vec4* b)
{
    out->x = MIN(a->x, b->x);
    out->y = MIN(a->y, b->y);
    out->z = MIN(a->z, b->z);
    out->w = MAX(a->w, b->w);
    return out;
}
extern MATHF_IMPL vec4* Vec4Max(vec4* out, const vec4* a, const vec4* b)
{
    out->x = MAX(a->x, b->x);
    out->y = MAX(a->y, b->y);
    out->z = MAX(a->z, b->z);
    out->w = MAX(a->w, b->w);
    return out;
}



/*
   Matrices.
*/

MATHF_IMPL void Mat4Load(mat4 *out, float data[16])
{
    for (int i = 0; i < 16; ++i) {
        out->c[16] = data[i];
    }
}

MATHF_IMPL mat4* Mat4Ident(mat4 *out, float ident)
{
	out->c[0] = ident; out->c[1] = 0; out->c[2] = 0; out->c[3] = 0;
	out->c[4] = 0; out->c[5] = ident; out->c[6] = 0; out->c[7] = 0;
	out->c[8] = 0; out->c[9] = 0; out->c[10] = ident; out->c[11] = 0;
	out->c[12] = 0; out->c[13] = 0; out->c[14] = 0; out->c[15] = ident;
	return out;
}

MATHF_IMPL mat4* Mat4FromTranslation(mat4 *out, const vec4 *a)
{
	Mat4Ident(out, 1.0f);
	out->c[12] = a->x;
	out->c[13] = a->y;
	out->c[14] = a->z;
	return out;
}

MATHF_IMPL mat4* Mat4FromScale(mat4 *out, const vec4 *a)
{
	Mat4Ident(out, 1.0f);
	out->c[0] = a->x;
	out->c[5] = a->y;
	out->c[10] = a->z;
	return out;
}

MATHF_IMPL mat4* Mat4FromRotation(mat4 *out, const vec4 *a)
{
	float x = a->x;
	float y = a->y;
	float z = a->z;
	float w = a->w;

	float tx = x + x; /* Note: Using x + x instead of 2.0f * x to force this function to return the same value as the SSE4.1 version across platforms. */
	float ty = y + y;
	float tz = z + z;

	float xx = tx * x;
	float yy = ty * y;
	float zz = tz * z;
	float xy = tx * y;
	float xz = tx * z;
	float xw = tx * w;
	float yz = ty * z;
	float yw = ty * w;
	float zw = tz * w;

	out->c[0] = (1.0f - yy) - zz;
	out->c[1] = xy + zw;
	out->c[2] = xz - yw;
	out->c[3] = 0;
	out->c[4] = xy - zw;
	out->c[5] = (1.0f - zz) - xx;
	out->c[6] = yz + xw;
    out->c[7] = 0;
	out->c[8] = xz + yw;
	out->c[9] = yz - xw;
	out->c[10] = (1.0f - xx) - yy;
	out->c[11] = 0;
	out->c[12] = 0;
	out->c[13] = 0;
	out->c[14] = 0;
	out->c[15] = 1.0f;
	return out;
}

MATHF_IMPL mat4* Mat4Translate(mat4 *out, const mat4 *a, const vec4 *b)
{
	out->c[12] += a->c[0] * b->x + a->c[4] * b->y + a->c[8] * b->z;
	out->c[13] += a->c[1] * b->x + a->c[5] * b->y + a->c[9] * b->z;
	out->c[14] += a->c[2] * b->x + a->c[6] * b->y + a->c[10] * b->z;
	return out;
}

MATHF_IMPL mat4* Mat4Scale3(mat4 *out, const mat4 *a, const vec4 *b)
{
	for (int i = 0; i < 3; i++)
    {
		out->c[0 + i] = a->c[0 + i] * b->x;
		out->c[4 + i] = a->c[4 + i] * b->y;
		out->c[8 + i] = a->c[8 + i] * b->z;
	}
	return out;
}
MATHF_IMPL mat4* Mat4Scale4(mat4 *out, const mat4 *a, const vec4 *b)
{
	for (int i = 0; i < 4; i++)
    {
		out->c[0 + i] = a->c[0 + i] * b->x;
		out->c[4 + i] = a->c[4 + i] * b->y;
		out->c[8 + i] = a->c[8 + i] * b->z;
	    out->c[12 + i] = a->c[12 + i] * b->w;
	}
	return out;
}


MATHF_IMPL mat4* Mat4Rotate(mat4 *out, const mat4 *a, const vec4 *b)
{
	mat4 rot;
	Mat4FromRotation(&rot, b);
    /* Rotation is accumulative. */
	return Mat4Mul(out, a, &rot);
}

MATHF_IMPL mat4 *Mat4Inverse3(mat4 *out, const mat4 *m)
{
	float det;
	float a = m->c[0], b = m->c[1], c = m->c[2],
		  d = m->c[4], e = m->c[5], f = m->c[6],
		  g = m->c[8], h = m->c[9], i = m->c[10];

	out->c[0] = e * i - f * h;
	out->c[1] = -(b * i - h * c);
	out->c[2] = b * f - e * c;
	out->c[3] = 0;
	out->c[4] = -(d * i - g * f);
	out->c[5] = a * i - c * g;
	out->c[6] = -(a * f - d * c);
	out->c[7] = 0;
	out->c[8] = d * h - g * e;
	out->c[9] = -(a * h - g * b);
	out->c[10] = a * e - b * d;
	out->c[11] = 0;
	out->c[12] = 0;
	out->c[13] = 0;
	out->c[14] = 0;
	out->c[15] = 0;

    // Calculate inverse determinant
	det = 1.0f / (a * out->c[0] + b * out->c[4] + c * out->c[8]);
	for (int i = 0; i < 16; ++i)
    {
		out->c[i] *= det;
	}
	return out;
}

MATHF_IMPL mat4 *Mat4Transpose3(mat4 *out, const mat4 *m)
{
	out->c[0] = m->c[0];
	out->c[1] = m->c[4];
	out->c[2] = m->c[8];
	out->c[3] = 0;
	out->c[4] = m->c[1];
	out->c[5] = m->c[5];
	out->c[6] = m->c[9];
	out->c[7] = 0;
	out->c[8] = m->c[2];
	out->c[9] = m->c[6];
	out->c[10] = m->c[10];
	out->c[11] = 0;
	out->c[12] = 0;
	out->c[13] = 0;
	out->c[14] = 0;
	out->c[15] = 1;
	return out;
}

MATHF_IMPL mat4 *Mat4TransposePlace4(mat4 *out)
{
    float tmp;

    for (size_t i = 0; i < 4; ++i)
    {
        for (size_t j = i + 1; j < 4; ++j)
        {
            // Swap.
            tmp = out->c[4*i + j];
            out->c[4*i + j] = out->c[4*j + i];
            out->c[4*j + i] = tmp;
        }
    }

    return out;
}

MATHF_IMPL mat4* Mat4Transpose4(mat4* out, const mat4* m)
{
    if (out == m)
    {
        return Mat4TransposePlace4(out);
    }

	out->c[0] = m->c[0];
	out->c[1] = m->c[4];
	out->c[2] = m->c[8];
	out->c[3] = m->c[12];

	out->c[4] = m->c[1];
    out->c[5] = m->c[5];
	out->c[6] = m->c[9];
	out->c[7] = m->c[13];

	out->c[8] = m->c[2];
	out->c[9] = m->c[6];
	out->c[10] = m->c[10];
	out->c[11] = m->c[14];

	out->c[12] = m->c[3];
	out->c[13] = m->c[7];
	out->c[14] = m->c[11];
	out->c[15] = m->c[15];

	return out;
}



/*
   The VP matrices.
*/

MATHF_IMPL mat4 *Mat4LookAt(mat4 *out, const vec4 *eye, const vec4 *center, const vec4 *up)
{
	vec4 f, s, u;
	Vec4Nor3(&f, Vec4Sub(&f, center, eye));
	Vec4Nor3(&s, Vec4Cross3(&s, &f, up));
	Vec4Cross3(&u, &s, &f);

	out->c[0] = s.x;
	out->c[1] = u.x;
	out->c[2] = -f.x;
	out->c[3] = 0;
	out->c[4] = s.y;
	out->c[5] = u.y;
	out->c[6] = -f.y;
	out->c[7] = 0;
	out->c[8] = s.z;
	out->c[9] = u.z;
	out->c[10] = -f.z;
	out->c[11] = 0;
	out->c[12] = -Vec4Dot3(&s, eye);
	out->c[13] = -Vec4Dot3(&u, eye);
	out->c[14] = Vec4Dot3(&f, eye);
	out->c[15] = 1;
	return out;
}
MATHF_IMPL mat4* Mat4Look(mat4* out, const vec4* eye, const vec4* dir, const vec4* up)
{
	vec4 center;
	Vec4Add(&center, eye, dir);
	return Mat4LookAt(out, eye, &center, up);
}

MATHF_IMPL mat4* Mat4Perspective(mat4 *out, float fovy, float aspect, float fnear, float ffar)
{
	float f = 1.0f / tanf(fovy * 0.5f);
	float fn = 1.0f / (fnear - ffar);
	Mat4Ident(out, 0.0f);
	out->c[0] = f / aspect;
	out->c[5] = f;
	out->c[10] = ffar * fn;
	out->c[11] = -1.0f;
	out->c[14] = fnear * ffar * fn;
	return out;
}

MATHF_IMPL mat4* Mat4Ortho(mat4 *out, float left, float right, float bottom, float top, float fnear, float ffar)
{

    float rl = (float)(right - left);
    float tb = (float)(top - bottom);
    float fn = (float)(ffar - fnear);

	Mat4Ident(out, 0);
    out->c[0 ] = 2/rl;
    out->c[1 ] = 0;
    out->c[2 ] = 0;
    out->c[3 ] = 0;
    out->c[4 ] = 0;
    out->c[5 ] = 2/tb;
    out->c[6 ] = 0;
    out->c[7 ] = 0;
    out->c[8 ] = 0;
    out->c[9 ] = 0;
    out->c[10] = -2/fn;
    out->c[11] = 0;
    out->c[12] = -((float)left + (float)right)/rl;
    out->c[13] = -((float)top + (float)bottom)/tb;
    out->c[14] = -((float)ffar + (float)fnear)/fn;
    out->c[15] = 1;

    return out;
}


/*
   Left-handed variants.
*/

MATHF_IMPL mat4 *Mat4LookAt_LH(mat4 *out, const vec4 *eye, const vec4 *center, const vec4 *up)
{
    vec4 f, s, u;
    Vec4Nor3(&f, Vec4Sub(&f, center, eye));
    Vec4Nor3(&s, Vec4Cross3(&s, &f, up));
    Vec4Cross3(&u, &s, &f);

    // Inverse of the f axis.
    out->c[0] = s.x;
    out->c[1] = u.x;
    out->c[2] = -f.x;
    out->c[3] = 0;
    out->c[4] = s.y;
    out->c[5] = u.y;
    out->c[6] = -f.y;
    out->c[7] = 0;
    out->c[8] = s.z;
    out->c[9] = u.z;
    out->c[10] = -f.z;
    out->c[11] = 0;
    out->c[12] = -Vec4Dot3(&s, eye);
    out->c[13] = -Vec4Dot3(&u, eye);
    out->c[14] = Vec4Dot3(&f, eye);
    out->c[15] = 1;
    return out;
}
MATHF_IMPL mat4* Mat4Look_LH(mat4* out, const vec4* eye, const vec4* dir, const vec4* up)
{
	vec4 center;
	Vec4Add(&center, eye, dir);
	return Mat4LookAt_LH(out, eye, &center, up);
}

MATHF_IMPL mat4* Mat4Perspective_LH(mat4 *out, float fovy, float aspect, float fnear, float ffar)
{
	float f = 1.0f / tanf(fovy * 0.5f);
	float fn = 1.0f / (fnear - ffar);
	Mat4Ident(out, 0.0f);
	out->c[0] = f / aspect;
	out->c[5] = f;
	out->c[10] = ffar * fn;
	out->c[11] = -1.0f; // 1 for Right-Handed.
	out->c[14] = fnear * ffar * fn;
	return out;
}

MATHF_IMPL mat4* Mat4Ortho_LH(mat4 *out, float left, float right, float bottom, float top, float fnear, float ffar)
{

    float rl = (float)(right - left);
    float tb = (float)(top - bottom);
    float fn = (float)(ffar - fnear);

	Mat4Ident(out, 0);
    out->c[0 ] = 2/rl;
    out->c[1 ] = 0;
    out->c[2 ] = 0;
    out->c[3 ] = 0;
    out->c[4 ] = 0;
    out->c[5 ] = 2/tb;
    out->c[6 ] = 0;
    out->c[7 ] = 0;
    out->c[8 ] = 0;
    out->c[9 ] = 0;
    out->c[10] = -1/fn;
    out->c[11] = 0;
    out->c[12] = -((float)left + (float)right)/rl; // Negative numbers (-) for translation in Right-Handed.
    out->c[13] = -((float)top + (float)bottom)/tb;
    out->c[14] = -fnear / (ffar - fnear);
    out->c[15] = 1;

    return out;
}





MATHF_IMPL vec4 *Mat4GetRotation(vec4 *out, const mat4 *mat)
{
	const float *m = mat->c;

	float tr = m[0] + m[5] + m[10];
	if (tr >= 0.0f)
    {
		float s = sqrt(tr + 1.0f);
		float is = 0.5f / s;
		out->x = (m[1 * 4 + 2] - m[2 * 4 + 1]) * is;
		out->y = (m[2 * 4 + 0] - m[0 * 4 + 2]) * is;
		out->z = (m[0 * 4 + 1] - m[1 * 4 + 0]) * is;
		out->w = 0.5f * s;
		return out;
	}

	int i = 0;
	if (m[1 * 4 + 1] > m[0 * 4 + 0]) i = 1;
	if (m[2 * 4 + 2] > m[i * 4 + i]) i = 2;

	if (i == 0)
    {
		float s = sqrt(m[0] - (m[5] + m[10]) + 1);
		float is = 0.5f / s;
		out->x = 0.5f * s;
		out->y = (m[1 * 4 + 0] + m[0 * 4 + 1]) * is;
		out->z = (m[0 * 4 + 2] + m[2 * 4 + 0]) * is;
		out->w = (m[1 * 4 + 2] - m[2 * 4 + 1]) * is;
		return out;
	} else if (i == 1)
    {
        float s = sqrt(m[5] - (m[10] + m[0]) + 1);
        float is = 0.5f / s;
        out->x = (m[1 * 4 + 0] + m[0 * 4 + 1]) * is;
        out->y = 0.5f * s;
        out->z = (m[2 * 4 + 1] + m[1 * 4 + 2]) * is;
        out->w = (m[2 * 4 + 0] - m[0 * 4 + 2]) * is;
        return out;
    } else
    {
        float s = sqrt(m[10] - (m[0] + m[5]) + 1);
        float is = 0.5f / s;
        out->x = (m[0 * 4 + 2] + m[2 * 4 + 0]) * is;
        out->y = (m[2 * 4 + 1] + m[1 * 4 + 2]) * is;
        out->z = 0.5f * s;
        out->w = (m[0 * 4 + 1] - m[1 * 4 + 0]) * is;
	}
	return out;
}

/*
   Printing/formatting.
*/


extern void PrintVec2(vec2 v)
{
    printf("VEC2(%f, %f)\n", v.x, v.y);
}
extern void PrintVec3(vec3 v)
{
    printf("VEC3(%f, %f, %f)\n", v.x, v.y, v.z);
}
extern void PrintVec4(vec4 v)
{
    printf("VEC4(%f, %f, %f, %f)\n", v.x, v.y, v.z, v.w);
}



extern void PrintMat2(mat4 mat)
{
    printf("MAT2:\n");
    for (size_t i = 0; i < 4; ++i) {
        printf("%f ", mat.c[i]);
        if (((i + 1) & 1) == 0) printf("\n");
    }
    printf("\n");
}

/* No bitwise & equivalent for the mod 3 operator? */
extern void PrintMat3(mat4 mat)
{
    printf("MAT3:\n");
    for (size_t i = 0; i < 8; ++i) {
        printf("%f ", mat.c[i]);
        if (((i + 1) % 3) == 0) printf("\n");
    }
    printf("\n");
}

/*
   Prints the truth (i.e. matrices are
   row-major specified in memory in C++).
*/
extern void PrintMat4(mat4 mat)
{
    printf("MAT4:\n");
    for (size_t i = 0; i < 16; ++i) {
        printf("%f ", mat.c[i]);
        if (((i + 1) & 3) == 0) printf("\n");
    }
    printf("\n");
}

/*
   Quaternions.
*/

// Right-handed quaternion convention.
extern MATHF_IMPL quat* QuatMul(quat* out, const quat* a, const quat* b)
{
    float x = a->w*b->x + a->x*b->w + a->y*b->z - a->z*b->y;
    float y = a->w*b->y - a->x*b->z + a->y*b->w + a->z*b->x;
    float z = a->w*b->z + a->x*b->y - a->y*b->x + a->z*b->w;
    float w = a->w*b->w - a->x*b->x - a->y*b->y - a->z*b->z;

    out->x = x;
    out->y = y;
    out->z = z;
    out->w = w;
    return out;
}

extern MATHF_IMPL quat* QuatNormalLerp(quat* out, const quat* a, const quat* b, float t)
{
    return Vec4Nor4(out, Vec4Lerp(out, a, b, t));
}

extern MATHF_IMPL quat* QuatSlerp(quat* out, const quat* a, const quat* b, float t)
{
    float cos = Vec4Dot4(a, b);
    if (cos > 0.99f)
    {
        // Rotation is very close.
        return QuatNormalLerp(out, a, b, t);
    }

    float theta = acosf(cos);
    quat v2;
    Vec4MulScalar(out, a, sinf(1.0f - t) * theta);
    Vec4MulScalar(&v2, b, sinf(t * theta));
    Vec4Add(out, out, &v2);
    Vec4DivScalar(out, out, sinf(theta));

    return out;
}

extern MATHF_IMPL quat* QuatAngleAxis(quat* out, const vec3* axis, float angle)
{
    float angle2 = angle * 0.5f;
    float s = sinf(angle2), c = cosf(angle2);
    out->x = axis->x * s;
    out->y = axis->y * s;
    out->z = axis->z * s;
    out->w = c;
    return out;
}

// v' = qvq'
extern MATHF_IMPL vec4* QuatRotate4(vec4* out, const quat* q, const vec4* v)
{
    quat conj;
    QuatConjugate(&conj, q);

    QuatMul(out, q, v);
    return QuatMul(out, out, &conj);
}

extern MATHF_IMPL vec3* QuatRotate3(vec3* out, const quat* q, const vec3* v)
{
    quat conj;
    QuatConjugate(&conj, q);

    vec4 expandV = CLITERAL(vec4) { v->x, v->y, v->z, 0 };
    vec4 expandOut = CLITERAL(vec4) { out->x, out->y, out->z, 0 };

    QuatMul(&expandOut, q, &expandV);
    QuatMul(&expandOut, &expandOut, &conj);

    *out = CLITERAL(vec3) { expandOut.x, expandOut.y, expandOut.z };
    return out;
}

// At this point you're just moving the issue away from the quaternion.
extern MATHF_IMPL quat* QuatEulerAngles_ZYX(quat* out, const vec3* angles)
{
    float cx = cosf(0.5f * angles->x), sx = sinf(0.5f * angles->x);
    float cy = cosf(0.5f * angles->y), sy = sinf(0.5f * angles->y);
    float cz = cosf(0.5f * angles->z), sz = sinf(0.5f * angles->z);

    out->x = cz*sx*cy - sz*cx*sy;
    out->y = cz*cx*sy + sz*sx*cy;
    out->z = sz*cx*cy - cz*sx*sy;
    out->w = cz*cx*cy + sz*sx*sy;
    return out;
}

extern MATHF_IMPL quat* QuatConjugate(quat* out, const quat* a)
{
    out->x = -a->x;
    out->y = -a->y;
    out->z = -a->z;
    out->w = a->w;
    return out;
}




/*
   Polynominal solvers.
*/

extern int SolveQuadratic(float sol[2], float a, float b, float c)
{
    if (sol == NULL)
    {
        return NULL;
    }

    if (ABS(a) < MATHF_F32_EPSILON || ABS(b) > 1e6f*ABS(a))
    {
        if (ABS(b) < MATHF_F32_EPSILON)
        {
            return 0;
        }

        sol[0] = -c/b;
        return 1;
    }

    float disc = b*b - 4*a*c;
    if (disc > 0)
    {
        float sqdisc = Sqrt(disc);

        sol[0] = (-b + sqdisc) / (2.0f * a);
        sol[1] = (-b - sqdisc) / (2.0f * a);
        return 2;
    } else if (ABS(disc) < MATHF_F32_EPSILON)
    {

        sol[0] = -b / (2.0f * a);
        return 1;
    }

    return 0;
}

extern int SolveCubicNormed(float sol[3], float a, float b, float c)
{
    float s = -a / 3;
    float p = b - a*a / 3;
    float q = a * (2*a*a - 9*b) / 27 + c;
    float p3 = p*p*p;
    float d = q*q + 4*p3 / 27;
    if (d >= 0)
    {
        float z = (float) sqrtf(d);
        float u = (-q + z) / 2;
        float v = (-q - z) / 2;
        u = cbrtf(u);
        v = cbrtf(v);
        sol[0] = s + u + v;
        return 1;
    }
    else
    {
        float u = (float) sqrtf(-p/3);
        float v = (float) acosf(-sqrtf(-27/p3) * q / 2) / 3;
        float m = (float) cosf(v);
        float n = (float) cosf(v-3.141592/2)*1.732050808f;
        sol[0] = s + u * 2 * m;
        sol[1] = s - u * (m + n);
        sol[2] = s - u * (m - n);

        return 3;
    }

    return 0;
}

extern int SolveCubic(float sol[3], float a, float b, float c, float d)
{
    if (sol == NULL)
    {
        return 0;
    }

    if (ABS(a) >= MATHF_F32_EPSILON)
    {
        float ba = b / a;
        if (ABS(ba) < 1e6f)
        {
            return SolveCubicNormed(sol, ba, c/a, d/a);
        }
    }

    return SolveQuadratic(sol, b, c, d);
}



/*
   Standard math functions.
*/

MATHF_IMPL f32 TruncF(f32 x)
{
#ifdef MATHF_USE_SSE
    __m128 floatVec = _mm_set_ss(x);

    //__m128d vec = reinterpret_cast<__m128d>(_mm_set_ss(x));
    //__m128d doubleVec = _mm_cvtps_pd(floatVec);

    int i = _mm_cvttss_si32(floatVec);
    return (f32) i;

#else 
    // Handle NaN and infinity.
    if (x != x || x == INFINITY || x == -INFINITY)
    {
        return x;
    }

    /* Extract sigh bit. */
    uint32_t bits;
    memcpy(&bits, &x, sizeof(x));
    int sign = (bits >> 31) & 1;

    uint32_t mask = 0xFFFFF000; /* Keep sign and exponent. Clear mantissa. */
    uint32_t int_bits = bits & mask;

    if ((bits & 0x7F800000) == 0)
    {
        return sign ? -0.0f : 0.0f;
    }

    f32 result;
    memcpy(&result, &int_bits, sizeof(result));

    if (result && result != x)
    {
        result += 1.0f;
    }

    return result;
#endif
}

// My Ryzen CPU uses SSE4a.
MATHF_IMPL f64 TruncD(f64 x)
{
#ifdef MATHF_USE_SSE
    /*
        SSE 4.1:
        return _mm_cvtsd_f64(_mm_round_sd(_mm_set_sd(x), _MM_FROUND_TRUNC));
    */

    /* Compatibility. */
    __m128d vec = _mm_set_sd(x);
    __m128i int_vec = _mm_cvttpd_epi32(vec);
    __m128d result = _mm_cvtepi32_pd(int_vec);
    return _mm_cvtsd_f64(result);
#else
    /* Handle NaN and infinity. */
    if (x != x || x == INFINITY || x == -INFINITY)
    {
        return x;
    }

    /* Extract sigh bit. */
    uint64_t bits;
    memcpy(&bits, &x, sizeof(x));
    int sign = (bits >> 63) & 1;

    uint64_t int_bits = bits & 0xFFFFFFFFFFFFF000;

    /* Subnormals (denormals). */
    if ((bits & 0x7FF0000000000000) == 0)
    {
        return sign ? -0.0 : 0.0;
    }

    f64 result;
    memcpy(&result, &int_bits, sizeof(result));

    if (result && result != x) {
        result += 1.0;
    }

    return result;
#endif
}

/*
 *  IEE 754 f32 floating-point representation:
 *  Bit  31      Sign
 *  Bits 30-23   Exponent
 *  Bits 22-00   Mantissa
 */
MATHF_IMPL int IsIntF(f32 x)
{
    int32_t i = *(int32_t *) &x;
    int exponent = ((i >> 23) & 0xFF) - 127;

    int bitsInFraction = 23 - exponent;
    int32_t mask = exponent < 0
                    ? 0x7FFFFFFF
                    : exponent > 23
                         ? 0x00
                         : (1 << bitsInFraction) - 1;

    return !(i & mask);
}


MATHF_IMPL int IsIntD(f64 x)
{
    uint64_t i = *(uint64_t *) &x;

    int exponent = ((i >> 52) & 0x7FF) - 1023;

    int bitsInFraction = 52 - exponent;
    uint64_t mask = exponent < 0
                    ? 0x7FFFFFFFFFFFFFFFLL
                    : exponent > 52
                         ? 0x00
                         : (1LL << bitsInFraction) - 1;

    return !(i & mask);
}


MATHF_IMPL f32 FloorF(f32 x) {
    if (IsIntF(x)) return x;
    if (x < 0) x -= 1;
    return TruncF(x);
}
MATHF_IMPL f64 FloorD(f64 x)
{
    if (IsIntD(x)) return x;
    if (x < 0) x -= 1;
    return TruncD(x);
}

MATHF_IMPL f32 CeilF(f32 x)
{
    if (IsIntF(x)) return x;
    if (x < 0) return TruncF(x);
    return TruncF(x) + 1;
}
MATHF_IMPL f64 CeilD(f64 x)
{
    if (IsIntD(x)) return x;
    if (x < 0) return TruncD(x);
    return TruncD(x) + 1;
}



/*
   Fast operations.
*/

/*
 *  QIIIA inverse square root can be adapted to return 1/x by doing (1/sqrt(x)) ^ 2,
 *  which is faster than hardware division.
 * Hardware division is an inherently iterative process that cannot be parallelized.
 */

MATHF_IMPL f32 Fast_Inv(f32 x)
{
    /*
       Best polynominal for log-accuracy: 0x7ef08eb3.
       Best polynominal for linear-accuracy: 0x7ef08dcb.
    */
    long i = 0x7ef08eb3 - ((*(u32 *) &x));
    f32 f = *(f32 *) &(i);
    return f * (2 - x*f);
}

/* x * Q_rsqrt. */
MATHF_IMPL f32 Fast_Sqrt(f32 x)
{
    long i = 0x5f3759df - ((*(u32 *) &x) >> 1);
    f32 f = *(f32 *) &(i);
    return x * f * (1.5 - 0.5 * x * f * f);
}

/* Q_rsqrt. */
MATHF_IMPL f32 Fast_InvSqrt(f32 x)
{
    long i = 0x5f3759df - ((*(u32 *) &x) >> 1);
    f32 f = *(f32 *) &(i);
    return f * (1.5 - 0.5 * x * f * f);
}

/*
   Wrappers.
*/

MATHF_IMPL f32 Inv(f32 x)
{
#ifdef MATHF_USE_FAST
    /*
       Dividing 1/x is still computationally hard,
       because division is an inherently iterative operation
       that cannot easily be parallelized by hardware.
    */
    return Fast_Inv(x);
#else
    return 1 / x;
#endif
}

MATHF_IMPL f32 InvSqrt(f32 x)
{
#ifdef MATHF_USE_SSE
    /* Approximates ~ 1/sqrt(x). */
    __m128 in = _mm_load_ss(&x);
    __m128 approx = _mm_rsqrt_ss(in);

    /* Refine the approx with Newton-Raphson method. */
    return _mm_cvtss_f32(approx);
#elif MATHF_USE_FAST
    return Fast_InvSqrt(x);
#else
    return 1 / (float) sqrt(x);
#endif
}

MATHF_IMPL f32 Sqrt(f32 x)
{
#ifdef MATHF_USE_SSE
    __m128 in = _mm_load_ss(&x);
    __m128 approx = _mm_sqrt_ss(in);
    return _mm_cvtss_f32(approx);
#elif MATHF_USE_FAST
    return Fast_Sqrt(x);
#else
    return (float) sqrt(x);
#endif
}


/*
   Doom
*/


MATHF_IMPL f32 Div(f32 a, f32 b)
{
    return (a) * Fast_Inv(b);
}



MATHF_IMPL f32 ModF(f32 a, f32 b)
{
    return a + TruncF(a / b) * b;
}

MATHF_IMPL f64 ModD(f64 a, f64 b)
{
    return a + TruncD(a / b) * b;
}


MATHF_IMPL f32 Sin(f32 angle)
{
    /* Normalize angle. */
    angle = ModF(angle, PI2);
    if (angle < 0) angle += PI2;

    /* Scale angle. */
    size_t idx = (size_t) (angle * TRIG_CACHE_SIZE / PI2);
    idx %= TRIG_CACHE_SIZE;

    return g_sin_cache[idx];
}

MATHF_IMPL f32 Cos(f32 angle)
{
    /* Normalize angle. */
    angle = ModF(angle, PI2);
    if (angle < 0) angle += PI2;

    /* Scale angle. */
    size_t idx = (size_t) (angle * TRIG_CACHE_SIZE / PI2);
    idx %= TRIG_CACHE_SIZE;

    return g_cos_cache[idx];
}

MATHF_IMPL u8 Rand_Byte(void)
{
    return 67;
    //return (rand() & 255);
}



extern MATHF_IMPL f32 GetVolumeSphere(f32 radius)
{
    return 4/3.0 * PI * radius*radius*radius;
}

extern MATHF_IMPL f32 GetVolumeBox(vec3 min, vec3 max)
{
    f32 dx = ABS(min.x - max.x);
    f32 dy = ABS(min.y - max.y);
    f32 dz = ABS(min.z - max.z);

    return dx*dy*dz;
}

extern MATHF_IMPL f32 GetVolumeBoxSize(f32 w, f32 h, f32 d)
{
    return ABS(w)*ABS(h)*ABS(d);
}

extern MATHF_IMPL f32 GetVolumeCylinder(f32 radius, f32 height)
{
    return height * PI * radius*radius;
}

extern MATHF_IMPL f32 GetVolumePrism(f32 radius, size_t sides, f32 height)
{
    f32 angle = PI2 / ((float) sides);
    return height * sides * Sin(angle) * radius * radius * 0.5f;
}


/*
   Areas/surfaces.
*/

extern MATHF_IMPL f32 GetAreaCircle(f32 radius)
{
    return PI * radius * radius;
}
extern MATHF_IMPL f32 GetAreaSquare(f32 side)
{
    return side*side;
}

extern MATHF_IMPL f32 GetAreaRect(vec2 min, vec2 max)
{
    f32 dx = min.x - max.x;
    f32 dy = min.y - max.y;
    return dx*dy;
}

extern MATHF_IMPL f32 GetAreaRectSize(f32 w, f32 h)
{
    return w * h;
}
extern MATHF_IMPL f32 GetAreaRegular(size_t sides, f32 radius)
{
    f32 angle = PI2 / ((float) sides);
    return sides * Sin(angle) * radius * radius * 0.5f;
}


extern MATHF_IMPL f32 GetAreaTriangle(vec3 a, vec3 b, vec3 c)
{
    vec3 ab; Vec3Sub(&ab, &b, &a);
    vec3 ac; Vec3Sub(&ac, &c, &a);
    vec3 cr; Vec3Cross(&cr, &ab, &ac);

    return 0.5f * Vec3Len(&cr);
}

extern MATHF_IMPL f32 GetAreaParallelogram(vec3 a, vec3 b, vec3 c)
{
    vec3 ab; Vec3Sub(&ab, &b, &a);
    vec3 ac; Vec3Sub(&ac, &c, &a);
    vec3 cr; Vec3Cross(&cr, &ab, &ac);

    return Vec3Len(&cr);
}






/*
    Perimeter.
*/

extern MATHF_IMPL f32 GetPerimCircle(f32 radius)
{
    return PI2 * radius;
}
extern MATHF_IMPL f32 GetPerimSquare(f32 side)
{
    return 4 * ABS(side);
}

extern MATHF_IMPL f32 GetPerimRect(vec2 min, vec2 max)
{
    f32 dx = min.x - max.x;
    f32 dy = min.y - max.y;
    return 2 * (ABS(dx) + ABS(dy));
}
extern MATHF_IMPL f32 GetPerimRectSize(f32 w, f32 h)
{
    return 2 * (ABS(w) + ABS(h));
}
extern MATHF_IMPL f32 GetPerimRegular(size_t sides, f32 radius)
{
    return sides * 2 * Sin(PI / (float) sides) * radius;
}

extern MATHF_IMPL f32 GetPerimTriangle(vec3 a, vec3 b, vec3 c)
{
    f32 d1 = Vec3Dst(&a, &b);
    f32 d2 = Vec3Dst(&b, &c);
    f32 d3 = Vec3Dst(&c, &a);

    return d1 + d2 + d3;
}


/*
   Raycasting.
*/

extern MATHF_IMPL RAYCASTRESULT RaycastPlaneDistance(RAY ray, float distance, vec3 normal)
{
    RAYCASTRESULT out = { 0 };

    float denom = Vec3Dot3(&ray.direction, &normal);
    // See if ray is parallel with the plane.
    if (denom == 0.0f)
    {
        return out;
    }

    float numerator = (distance - Vec3Dot3(&ray.origin, &normal) / Vec3Dot3(&ray.direction, &normal));
    out.t[0] = numerator / denom;

    // Plane is behind ray origin.
    if (out.t[0] < 0.0f)
    {
        return out;
    }

    out.count = 1;

    return out;
}

extern MATHF_IMPL RAYCASTRESULT RaycastPlane(RAY ray, vec3 center, vec3 normal)
{
    return RaycastPlaneDistance(ray, Vec3Dst3(&ray.origin, &center), normal);
}

extern MATHF_IMPL RAYCASTRESULT RaycastSphere(RAY ray, vec3 center, float radius)
{
    float t0, t1;
    RAYCASTRESULT out = { 0 };

    vec3 l = ray.origin - center;
    float a = Vec3Dot3(&ray.direction, &ray.direction);
    float b = 2 * Vec3Dot3(&ray.direction, &l);
    float c = Vec3Dot3(&l, &l) - (radius * radius);

    /* Solve quadratic equation. */
    float discr = b*b - 4*a*c;
    if (discr < 0)
    {
        return out;
    }
    else if (discr == 0)
    {
        out.t[0] = -0.5 * b / a;
        out.count = 1;
    }
    else
    {
        float q = (b > 0) ? -0.5 * (b + Sqrt(discr)) : -0.5 * (b - Sqrt(discr));
        out.t[0] = q / a;
        out.t[1] = c / q;

        out.count = 2;
    }


    return out;
}

extern MATHF_IMPL RAYCASTRESULT RaycastBox(RAY ray, vec3 min, vec3 max)
{
    float tmin, tmax;
    float t1, t2;
    float tn, tf;
    RAYCASTRESULT out = { 0 };

    tmin = 0.0f;
    tmax = (float) UINT32_MAX;

    // X axis.
    float inv_x = 1 / ray.direction.x;
    t1 = (min.x - ray.origin.x) * inv_x;
    t2 = (max.x - ray.origin.x) * inv_x;
    tn = MIN(t1, t2);
    tf = MAX(t1, t2);

    tmin = MAX(tmin, tn);
    tmax = MIN(tmax, tf);

    if (tmin > tf || tmax < tn)
    {
        return out;
    }

    // Y axis.
    float inv_y = 1 / ray.direction.y;
    t1 = (min.y - ray.origin.y) * inv_y;
    t2 = (max.y - ray.origin.y) * inv_y;
    tn = MIN(t1, t2);
    tf = MAX(t1, t2);

    tmin = MAX(tmin, tn);
    tmax = MIN(tmax, tf);

    if (tmin > tf || tmax < tn)
    {
        return out;
    }

    // Z axis.
    float inv_z = 1 / ray.direction.z;
    t1 = (min.z - ray.origin.z) * inv_z;
    t2 = (max.z - ray.origin.z) * inv_z;
    tn = MIN(t1, t2);
    tf = MAX(t1, t2);

    tmin = MAX(tmin, tn);
    tmax = MIN(tmax, tf);

    if (tmin > tf || tmax < tn)
    {
        return out;
    }

    if (tmin < 0)
    {
        out.t[0] = tmax;
        out.count = 1;
    }
    else
    {
        out.t[1] = tmin;
        out.count = 1;
    }

    return out;
}

/*
   Geometry.
*/

extern MATHF_IMPL vec3 GetLinePoint_MinPosition(vec3 Point, vec3 LinePos1, vec3 LinePos2)
{
	float Length12, t;
	float Dot12_1P;
	vec3 Line12, Line1P;
	vec3 Result;

	Line12.x = LinePos2.x - LinePos1.x;
	Line12.y = LinePos2.y - LinePos1.y;
	Line12.z = LinePos2.z - LinePos1.z;
	Line1P.x = Point.x - LinePos1.x;
	Line1P.y = Point.y - LinePos1.y;
	Line1P.z = Point.z - LinePos1.z;
	Dot12_1P = Line12.x * Line1P.x + Line12.y * Line1P.y + Line12.z * Line1P.z;
	if (Dot12_1P <= 0.0f)
    {
		return LinePos1;
	}

	Length12 = Line12.x * Line12.x + Line12.y * Line12.y + Line12.z * Line12.z;
	if (Length12 == 0.0f)
    {
		return LinePos1;
	}

	if (Length12 < Dot12_1P)
    {
		return LinePos2;
	}

	t = Dot12_1P / Length12;
	Result.x = LinePos1.x + Line12.x * t;
	Result.y = LinePos1.y + Line12.y * t;
	Result.z = LinePos1.z + Line12.z * t;

	return Result;
}

extern vec3 GetTrianglePoint_MinPosition(vec3 Point, vec3 TrianglePos1, vec3 TrianglePos2, vec3 TrianglePos3)
{
	vec3 Line12, Line23, Line31, Line1P, Line2P, Line3P, Result;
	float Dot1P2, Dot1P3, Dot2P1, Dot2P3, Dot2PH, Dot3P1, Dot3P2, Dot3PH, OPA, OPB, OPC, Div, t, v, w;

	Vec3Sub(&Line12, &TrianglePos2, &TrianglePos1);
	Vec3Sub(&Line31, &TrianglePos1, &TrianglePos3);
	Vec3Sub(&Line1P, &Point,        &TrianglePos1);
	Dot1P2 = Vec3Inner(&Line12, &Line1P);
	Dot1P3 = Vec3Inner(&Line31, &Line1P);
	if (Dot1P2 <= 0.0f && Dot1P3 >= 0.0f) return TrianglePos1;

	Vec3Sub(&Line23, &TrianglePos3, &TrianglePos2);
	Vec3Sub(&Line2P, &Point,        &TrianglePos2);
	Dot2P1 = Vec3Inner(&Line12, &Line2P);
	Dot2P3 = Vec3Inner(&Line23, &Line2P);
	if (Dot2P1 >= 0.0f && Dot2P3 <= 0.0f) return TrianglePos2;

	Dot2PH = Vec3Inner(&Line31, &Line2P);
	OPC = Dot1P2 * -Dot2PH - Dot2P1 * -Dot1P3 ;
    if (OPC <= 0.0f && Dot1P2 >= 0.0f && Dot2P1 <= 0.0f) {
        t = Dot1P2 / ( Dot1P2 - Dot2P1 ) ;
        Result.x = TrianglePos1.x + Line12.x * t;
        Result.y = TrianglePos1.y + Line12.y * t;
        Result.z = TrianglePos1.z + Line12.z * t;
        return Result;
    }

    Vec3Sub(&Line3P, &Point,        &TrianglePos3);
    Dot3P1 = Vec3Inner(&Line31, &Line3P);
    Dot3P2 = Vec3Inner(&Line23, &Line3P);
    if (Dot3P1 <= 0.0f && Dot3P2 >= 0.0f) return TrianglePos3;

    Dot3PH = Vec3Inner(&Line12, &Line3P);
    OPB = Dot3PH * -Dot1P3 - Dot1P2 * -Dot3P1;
    if (OPB <= 0.0f && Dot1P3 <= 0.0f && Dot3P1 >= 0.0f)
    {
        t = Dot3P1 / ( Dot3P1 - Dot1P3 );
        Result.x = TrianglePos3.x + Line31.x * t;
        Result.y = TrianglePos3.y + Line31.y * t;
        Result.z = TrianglePos3.z + Line31.z * t;
        return Result;
    }
    OPA = Dot2P1 * -Dot3P1 - Dot3PH * -Dot2PH;

    if (OPA <= 0.0f && ( -Dot2PH - Dot2P1 ) >= 0.0f && ( Dot3PH + Dot3P1 ) >= 0.0f)
    {
        t = (-Dot2PH - Dot2P1) / ((-Dot2PH - Dot2P1 ) + ( Dot3PH + Dot3P1 ));
        Result.x = TrianglePos2.x + Line23.x * t;
        Result.y = TrianglePos2.y + Line23.y * t;
        Result.z = TrianglePos2.z + Line23.z * t;
        return Result;
    }

    Div = 1.0f / (OPA + OPB + OPC);
    /* Barycentric coordinates. */
    v = OPB * Div;
    w = OPC * Div;
    Result.x = TrianglePos1.x + Line12.x * v - Line31.x * w;
    Result.y = TrianglePos1.y + Line12.y * v - Line31.y * w;
    Result.z = TrianglePos1.z + Line12.z * v - Line31.z * w;
	return Result;
}

extern MATHF_IMPL vec3 GetSpherePoint_MinPosition(vec3 point, vec3 center, float radius)
{
    vec3 out;
    Vec3Sub(&out, &point, &center);
    Vec3Nor3(&out, &out);
    Vec3MulScalar(&out, &out, radius);
    Vec3Add(&out, &out, &center);

    return out;
}

extern MATHF_IMPL vec3 GetSpherePoint_MaxPosition(vec3 point, vec3 center, float radius)
{
    vec3 out;
    Vec3Sub(&out, &center, &point);
    Vec3Nor3(&out, &out);
    Vec3MulScalar(&out, &out, radius);
    Vec3Add(&out, &out, &center);

    return out;
}

/*
   SDFs.
*/
extern MATHF_IMPL float GetSDSphere_Squared(vec3 point, vec3 center, float radius)
{
    float out;
    out = Vec3Dst3Squared(&point, &center);
    out -= (radius * radius);

    return out;
}

extern MATHF_IMPL float GetSDEllipsoid_Squared(vec3 point, vec3 center, vec3 sizes)
{
    float out;
    vec3 c; Vec3Sub(&c, &point, &center);
    Vec3Div(&c, &c, &sizes);

    out = c.x*c.x + c.y*c.y + c.z*c.z;
    // If the sum is 1, then the point is on the surface of the ellipsoid.

    return out;
}


/*
   Computational geometry.
*/

extern MATHF_IMPL int Points_Collinear(vec2 a, vec2 b, vec2 c)
{
    float determinant = a.x * (b.y - c.y) +
                        b.x * (c.y - a.y) +
                        c.x * (a.y - b.y);
    return (determinant == 0.0f);
}
extern MATHF_IMPL int Point_InsideTriangle(vec2 point, vec2 a, vec2 b, vec2 c)
{
    vec2 ba; Vec2Sub(&ba, &b, &a);
    vec2 cb; Vec2Sub(&cb, &c, &b);
    vec2 ac; Vec2Sub(&ac, &a, &c);
    vec2 pa; Vec2Sub(&pa, &point, &a);
    vec2 pb; Vec2Sub(&pb, &point, &b);
    vec2 pc; Vec2Sub(&pc, &point, &c);

    if (Vec2Cross(&ba, &pa) > 0.0f ||
        Vec2Cross(&ba, &pa) > 0.0f ||
        Vec2Cross(&ac, &pc) > 0.0f) {
            return 0;
    }
    return 1;
}
