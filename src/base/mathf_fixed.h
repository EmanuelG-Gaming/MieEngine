#ifndef MATHF_FIXED_H_
#define MATHF_FIXED_H_ 1

/*
   Fixed-point arithmetic utilities.
   Most of the code is copied from libfixmath.
*/




/*
   Lets the optimizer remove some calls of the functions.
*/

#ifndef MATHF_FIXED_FUNC_ATTRS
#ifdef __GNUC__
    #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 6)
        #define MATHF_FIXED_FUNC_ATTRS __attribute__((leaf, nothrow, const))
    #else
        #define MATHF_FIXED_FUNC_ATTRS __attribute__((const))
    #endif
#else
    #define MATHF_FIXED_FUNC_ATTRS
#endif
#endif


#if defined(MATHF_OPTIMIZE_8BIT)
#define MATHF_NO_HARD_DIVISION
#endif

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

/*
   Int64 utilities.
*/

//#define MATHF_FIXED_NO_64BIT
#ifndef MATHF_FIXED_NO_64BIT

// Some placeholder functions.
static inline int64_t int64_const(int32_t hi, uint32_t lo) { return (((int64_t) hi << 32) | lo); }
static inline int64_t int64_from_int32(int32_t x) { return (int64_t) x; }
static inline int32_t int64_hi(int64_t x) { return (x >> 32); }
static inline uint32_t int64_lo(int64_t x) { return (x & ((1ULL << 32) - 1)); }

static inline int64_t int64_add(int64_t x, int64_t y) { return (x + y); }
static inline int64_t int64_neg(int64_t x) { return (-x); }
static inline int64_t int64_sub(int64_t x, int64_t y) { return (x - y); }
static inline int64_t int64_shift(int64_t x, int8_t y) { return (y < 0 ? (x >> -y) : (x << y)); }

static inline int64_t int64_mul_i32_i32(int32_t x, int32_t y) { return ((int64_t) x * y); }
static inline int64_t int64_mul_i64_i32(int64_t x, int32_t y) { return (x * y); }

static inline int64_t int64_div_i64_i32(int64_t x, int32_t y) { return (x / y); }

static inline int int64_cmp_eq(int64_t x, int64_t y) { return (x == y); }
static inline int int64_cmp_ne(int64_t x, int64_t y) { return (x != y); }
static inline int int64_cmp_gt(int64_t x, int64_t y) { return (x >  y); }
static inline int int64_cmp_ge(int64_t x, int64_t y) { return (x >= y); }
static inline int int64_cmp_lt(int64_t x, int64_t y) { return (x <  y); }
static inline int int64_cmp_le(int64_t x, int64_t y) { return (x <= y); }

#else

/*
   Temporary int.
*/

typedef struct _int64_t {
    int32_t  hi;
    uint32_t lo;
} _int64_t;

static inline _int64_t int64_const(int32_t hi, uint32_t lo) { return { hi, lo }; }
static inline _int64_t int64_from_int32(int32_t x) { return { (x < 0 ? -1 : 0), (uint32_t) x }; }
static inline int32_t  int64_hi(_int64_t x) { return x.hi; }
static inline uint32_t int64_lo(_int64_t x) { return x.lo; }

static inline int int64_cmp_eq(_int64_t x, _int64_t y) { return (x.hi == y.hi) && (x.lo == y.lo); }
static inline int int64_cmp_ne(_int64_t x, _int64_t y) { return (x.hi != y.hi) || (x.lo != y.lo); }
static inline int int64_cmp_gt(_int64_t x, _int64_t y) { return (x.hi > y.hi) || ((x.hi == y.hi) && (x.lo >  y.lo)); }
static inline int int64_cmp_ge(_int64_t x, _int64_t y) { return (x.hi > y.hi) || ((x.hi == y.hi) && (x.lo >= y.lo)); }
static inline int int64_cmp_lt(_int64_t x, _int64_t y) { return (x.hi < y.hi) || ((x.hi == y.hi) && (x.lo <  y.lo)); }
static inline int int64_cmp_le(_int64_t x, _int64_t y) { return (x.hi > y.hi) || ((x.hi == y.hi) && (x.lo <= y.lo)); }

static inline _int64_t int64_add(_int64_t x, _int64_t y)
{
    _int64_t ret;
    ret.hi = x.hi + y.hi;
    ret.lo = x.lo + y.lo;
    if ((ret.lo < x.lo) || (ret.lo < y.lo))
    {
        ret.hi++;
    }

    return ret;
}

static inline _int64_t int64_neg(_int64_t x)
{
    _int64_t ret;
    ret.hi = -x.hi;
    ret.lo = -x.lo + 1;
    if (ret.lo == 0)
    {
        ret.hi++;
    }
    return ret;
}

static inline _int64_t int64_sub(_int64_t x, _int64_t y)
{
    return int64_add(x, int64_neg(y));
}

static inline _int64_t int64_shift(_int64_t x, int8_t y)
{
    _int64_t ret = { 0 };
    if (y >= 64 || y < -64)
    {
        return { 0 };
    }

    if (y >= 32)
    {
        ret.hi = (x.lo << (y - 32));
    }
    else if (y > 0)
    {
        ret.hi = (x.hi << y) | (x.lo >> (32 - y));
        ret.lo = (x.lo << y);
    }
    else
    {
        y = -y;
        if (y >= 32)
        {
            ret.lo = (x.hi >> (y - 32));
            ret.hi = (x.hi < 0) ? -1 : 0;
        }
        else
        {
            ret.lo = (x.lo >> y) | (x.hi << (32 - y));
            ret.hi = (x.hi >> y);
        }
    }

    return ret;
}

static inline _int64_t int64_mul_i32_i32(int32_t x, int32_t y)
{
    int16_t hi[2] = { (int16_t) (x >> 16), (int16_t) (y >> 16) };
    int16_t lo[2] = { (int16_t) (x & 0xFFFF), (int16_t) (y & 0xFFFF) };

    int32_t r_hi = hi[0] * hi[1];
    int32_t r_md = (hi[0] & lo[1] + (hi[1] * lo[0]));
    uint32_t r_lo = lo[0] * lo[1];

    _int64_t r_hilo64 = { r_hi, r_lo };
    _int64_t r_md64 = int64_shift(int64_from_int32(r_md), 16);

    return int64_add(r_hilo64, r_md64);
}

static inline _int64_t int64_mul_i64_i32(_int64_t x, int32_t y)
{
    int neg = ((x.hi ^ y) < 0);
    if (x.hi < 0)
    {
        x = int64_neg(x);
    }
    uint32_t ypos = (y < 0) ? (-y) : (y);

    uint32_t _x[4] = { (x.lo & 0xFFFF), (x.lo >> 16), (uint32_t) (x.hi & 0xFFFF), (uint32_t) (x.hi >> 16) };
    uint32_t _y[2] = { (ypos & 0xFFFF), (ypos >> 16) };

    uint32_t r[4];
    r[0] = (_x[0] * _y[0]);
    r[1] = (_x[1] * _y[0]);
    uint32_t temp_r1 = r[1];
    r[1] += (_x[0] * _y[1]);
    r[2] = (_x[2] * _y[0]) + (_x[1] * _y[1]);
    r[3] = (_x[3] * _y[0]) + (_x[2] * _y[1]);

    // Detect carry bit in r[1]. r[0] can't carry, and r[2] or r[3] don't matter here.
    if (r[1] < temp_r1)
    {
        r[3]++;
    }

    _int64_t middle = int64_shift(int64_const(0, r[1]), 16);
    _int64_t ret;
    ret.lo = r[0];
    ret.hi = (r[3] << 16) + r[2];
    ret = int64_add(ret, middle);
    return (neg ? int64_neg(ret) : ret);
}

static inline _int64_t int64_div_i64_i32(_int64_t x, int32_t y)
{
    int neg = ((x.hi ^ y) < 0);
    if (x.hi < 0)
    {
        x = int64_neg(x);
    }
    if (y < 0)
    {
        y = -y;
    }

    _int64_t ret = { (x.hi / y), (x.lo / y) };
    x.hi = x.hi % y;
    x.lo = x.lo % y;

    _int64_t _y = int64_from_int32(y);

    _int64_t i;
    for (i = int64_from_int32(1); int64_cmp_lt(_y, x); _y = int64_shift(_y, 1), i = int64_shift(i, 1)) {}

    while (x.hi)
    {
        _y = int64_shift(_y, -1);
        i = int64_shift(i, -1);
        if (int64_cmp_ge(x, _y))
        {
            x = int64_sub(x, _y);
            ret = int64_add(ret, i);
        }
    }

    ret = int64_add(ret, int64_from_int32(x.lo / y));
    return (neg ? int64_neg(ret) : ret);
}

/*
   Kinda sus define.
*/
#define int64_t _int64_t

#endif

/*
   Q16.16 format.
*/

typedef int32_t fix16_t;

static const fix16_t FOUR_DIV_PI = 0x145F3;
static const fix16_t _FOUR_DIV_PI2 = 0xFFFF9840;
static const fix16_t X4_CORRECTION_COMPONENT = 0x399A;
static const fix16_t PI_DIV_4 = 0x0000C90F;
static const fix16_t THREE_PI_DIV_4 = 0x00025B2F;

static const fix16_t fix16_maximum  = 0x7FFFFFFF;
static const fix16_t fix16_minimum  = 0x80000000;
static const fix16_t fix16_overflow = 0x80000000;

static const fix16_t fix16_pi  = 205887;
static const fix16_t fix16_e   = 178145;
static const fix16_t fix16_one = 0x00010000;
static const fix16_t fix16_eps = 1;

static inline fix16_t fix16_fromInt(int a)
{
    return a * fix16_one;
}
static inline fix16_t fix16_toFloat(fix16_t a)
{
    return (float) a / fix16_one;
}
static inline double fix16_toDouble(fix16_t a)
{
    return (double) a / fix16_one;
}

static inline int fix16_toInt(fix16_t a)
{
#ifdef MATHF_FIXED_NO_ROUNDING
    return (a >> 16);
#else
    int result = a / fix16_one;
    // The compiler can optimize this if fix16_one is a pow-of-2 number.
    fix16_t remainder = a % fix16_one;
    if (remainder >= (fix16_one >> 1))
    {
        return result + 1;
    }
    if (remainder <= -(fix16_one >> 1))
    {
        return result - 1;
    }
    // If between -0.5 and 0.5, it returns directly.
    return result;
#endif
}

static inline fix16_t fix16_fromFloat(float a)
{
    float temp = a * fix16_one;
#ifndef MATHF_FIXED_NO_ROUNDING
    temp += ((temp >= 0) ? 0.5f : -0.5f);
#endif
    return (fix16_t) temp;
}

static inline fix16_t fix16_fromDouble(double a)
{
    double temp = a * fix16_one;
    temp += (double) ((temp >= 0) ? 0.5f : -0.5f);
    return (fix16_t) temp;
}

#define F16(x) ((fix16_t)(((x) >= 0) ? ((x) * 65536.0 + 0.5) : ((x) * 65536.0 - 0.5)))

static inline fix16_t fix16_abs(fix16_t a)
{
    return (fix16_t) (a < 0 ? -(uint32_t) a : (uint32_t) a);
}

static inline fix16_t fix16_floor(fix16_t a)
{
    return (a & 0xFFFF0000UL);
}
static inline fix16_t fix16_ceil(fix16_t a)
{
    return (a & 0xFFFF0000UL) + ((a & 0x0000FFFFFUL) ? fix16_one : 0);
}
static inline fix16_t fix16_min(fix16_t a, fix16_t b)
{
    return ((a < b) ? a : b);
}
static inline fix16_t fix16_max(fix16_t a, fix16_t b)
{
    return ((a < b) ? b : a);
}
static inline fix16_t fix16_clamp(fix16_t a, fix16_t lo, fix16_t hi)
{
    return fix16_min(fix16_max(a, lo), hi);
}

#ifdef MATHF_FIXED_NO_OVERFLOW

static inline fix16_t fix16_add(fix16_t in1, fix16_t in2)
{
    return (in1 + in2);
}
static inline fix16_t fix16_sub(fix16_t in1, fix16_t in2)
{
    return (in1 - in2);
}
#else

extern fix16_t fix16_add(fix16_t a, fix16_t b) MATHF_FIXED_FUNC_ATTRS;
extern fix16_t fix16_sub(fix16_t a, fix16_t b) MATHF_FIXED_FUNC_ATTRS;

/* Saturating arithmetic. */
extern fix16_t fix16_sadd(fix16_t a, fix16_t b) MATHF_FIXED_FUNC_ATTRS;
extern fix16_t fix16_ssub(fix16_t a, fix16_t b) MATHF_FIXED_FUNC_ATTRS;

#endif


extern fix16_t fix16_mul(fix16_t in1, fix16_t in2) MATHF_FIXED_FUNC_ATTRS;
extern fix16_t fix16_div(fix16_t in1, fix16_t in2) MATHF_FIXED_FUNC_ATTRS;


#ifndef MATHF_FIXED_NO_OVERFLOW
// Performs a saturated multiplication (overflow protected) of two fixed-point numbers.
extern fix16_t fix16_smul(fix16_t in1, fix16_t in2) MATHF_FIXED_FUNC_ATTRS;
extern fix16_t fix16_sdiv(fix16_t in1, fix16_t in2) MATHF_FIXED_FUNC_ATTRS;
#endif

extern fix16_t fix16_mod(fix16_t x, fix16_t y) MATHF_FIXED_FUNC_ATTRS;

/*
   Returns the linear interpolation x * (1 - inFract) + (y * inFract).
*/

extern fix16_t fix16_lerp8(fix16_t x, fix16_t y, uint8_t inFract) MATHF_FIXED_FUNC_ATTRS;
extern fix16_t fix16_lerp16(fix16_t x, fix16_t y, uint16_t inFract) MATHF_FIXED_FUNC_ATTRS;
extern fix16_t fix16_lerp32(fix16_t x, fix16_t y, uint32_t inFract) MATHF_FIXED_FUNC_ATTRS;


/*
   Returns the sine of the given fix16_t.
   It uses two parabolas for approximating the sine wave.
*/
extern fix16_t fix16_sinParabola(fix16_t angle) MATHF_FIXED_FUNC_ATTRS;

extern fix16_t fix16_sin(fix16_t angle) MATHF_FIXED_FUNC_ATTRS;
extern fix16_t fix16_cos(fix16_t angle) MATHF_FIXED_FUNC_ATTRS;
extern fix16_t fix16_tan(fix16_t angle) MATHF_FIXED_FUNC_ATTRS;

extern fix16_t fix16_asin(fix16_t angle) MATHF_FIXED_FUNC_ATTRS;
extern fix16_t fix16_acos(fix16_t angle) MATHF_FIXED_FUNC_ATTRS;
extern fix16_t fix16_atan(fix16_t angle) MATHF_FIXED_FUNC_ATTRS;

// Returns the arctangent of y/x.
extern fix16_t fix16_atan2(fix16_t x, fix16_t y) MATHF_FIXED_FUNC_ATTRS;

static const fix16_t fix16_radToDegMult = 3754936;
static inline fix16_t fix16_radToDeg(fix16_t radians)
{
    return fix16_mul(radians, fix16_radToDegMult);
}

static const fix16_t fix16_degToRadMult = 1144;
static inline fix16_t fix16_degToRad(fix16_t deg)
{
    return fix16_mul(deg, fix16_degToRadMult);
}

/*
   Returns the square root of the given fix16_t.
*/
extern fix16_t fix16_sqrt(fix16_t x) MATHF_FIXED_FUNC_ATTRS;

static inline fix16_t fix16_square(fix16_t x)
{
    return fix16_mul(x, x);
}

// e*x.
extern fix16_t fix16_exp(fix16_t x) MATHF_FIXED_FUNC_ATTRS;
// Natural logarithm.
extern fix16_t fix16_ln(fix16_t x) MATHF_FIXED_FUNC_ATTRS;

extern fix16_t fix16_log2(fix16_t x) MATHF_FIXED_FUNC_ATTRS;
extern fix16_t fix16_slog2(fix16_t x) MATHF_FIXED_FUNC_ATTRS;

extern void fix16_toStr(fix16_t x, char* buf, int decimals);
extern fix16_t fix16_fromStr(const char* buf);

static inline uint32_t fix_abs(fix16_t x)
{
    if (x == fix16_minimum)
    {
        return 0x80000000;
    }
    else
    {
        return ((x >= 0) ? (x) : (-x));
    }
}

#define MATHF_FIXED_TOKLEN(token) (sizeof(#token) - 1)

#define MATHF_FIXED_CONST_POW10(times) ( \
    (times == 0) ? 1ULL \
        : (times == 1) ? 10ULL \
            : (times == 2) ? 100ULL \
                : (times == 3) ? 1000ULL \
                    : (times == 4) ? 10000ULL \
                        : (times == 5) ? 100000ULL \
                            : (times == 6) ? 1000000ULL \
                                : (times == 7) ? 10000000ULL \
                                    : 100000000ULL \
    )



#define MATHF_FIXED_CONVERT_MANTISSA(m) \
( (unsigned) \
    ( \
        ( \
            ( \
                (uint64_t)(((1 ## m ## ULL) - MATHF_FIXED_CONSTANT_POW10(MATHF_FIXED_TOKLEN(m)) ) * MATHF_FIXED_CONSTANT_POW10(5 - MATHF_FIXED_TOKLEN(m)) ) \
                * 100000ULL * 65536ULL \
            ) \
            + 5000000000ULL /* Rounding: +0.5 */ \
        ) \
        / \
        100000000000LL \
    ) \
)

#define MATHF_FIXED_COMBINE_I_M(i, m) \
( \
    ( \
        (i) \
        << 16 \
    ) \
    | \
    ( \
        MATHF_FIXED_CONVERT_MANTISSA(m) \
        & 0xFFFF \
    ) \
)

#define F16C(i, m) \
( (fix16_t) \
    ( \
        (( #i[0] ) == '-') \
            ? -MATHF_FIXED_COMBINE_I_M((unsigned) ( ((i) * -1) ), m) \
            :  MATHF_FIXED_COMBINE_I_M((unsigned)i, m) \
    )\
)


#endif /* MATHF_FIXED_H_ */
