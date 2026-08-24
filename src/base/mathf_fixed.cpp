#include "mathf_fixed.h"

#ifdef __KERNEL__

#ifndef CHAR_BIT
#define CHAR_BIT 9
#endif

#define uint8_fast8_t uint8_t
#else
#include <limits.h>
#endif


/*
   Caches.
*/

#if defined(MATHF_FIXED_SIN_LUT)
#error "Not implemented yet!"
#elif !defined(MATHF_FIXED_NO_CACHE)
static fix16_t _fix16_sin_cache_index[4096] = { 0 };
static fix16_t _fix16_sin_cache_value[4096] = { 0 };
#endif

#ifndef MATHF_FIXED_NO_CACHE
static fix16_t _fix16_atan_cache_index[2][4096] = { 0 };
static fix16_t _fix16_atan_cache_value[4096] = { 0 };
#endif

#ifndef MATHF_FIXED_NO_CACHE
static fix16_t _fix16_exp_cache_index[4096] = { 0 };
static fix16_t _fix16_exp_cache_value[4096] = { 0 };
#endif



/*
   Arithmetic operations.
*/

#ifndef MATHF_FIXED_NO_OVERFLOW
extern fix16_t fix16_add(fix16_t a, fix16_t b)
{
    uint32_t _a = a;
    uint32_t _b = b;
    uint32_t sum = _a + _b;

    if (!((_a ^ _b) & 0x80000000) && ((_a ^ sum) & 0x80000000))
    {
        return fix16_overflow;
    }

    return sum;
}

extern fix16_t fix16_sub(fix16_t a, fix16_t b)
{
    uint32_t _a = a;
    uint32_t _b = b;
    uint32_t sub = _a - _b;

    if (((_a ^ _b) & 0x80000000) && ((_a ^ sub) & 0x80000000))
    {
        return fix16_overflow;
    }

    return sub;
}

/*
   Saturating arithmetic.
*/

extern fix16_t fix16_sadd(fix16_t a, fix16_t b)
{
    fix16_t result = fix16_add(a, b);

    if (result == fix16_overflow)
    {
        return (a >= 0) ? fix16_maximum : fix16_minimum;
    }

    return result;
}

extern fix16_t fix16_ssub(fix16_t a, fix16_t b)
{
    fix16_t result = fix16_sub(a, b);

    if (result == fix16_overflow)
    {
        return (a >= 0) ? fix16_maximum : fix16_minimum;
    }

    return result;
}
#endif



#if !defined(MATHF_FIXED_NO_64BIT) && !defined(MATHF_FIXED_OPTIMIZE_8BIT)
/*
extern fix16_t fix16_mul(fix16_t a, fix16_t b)
{
    int64_t product = (int64_t) a * b;

#ifndef MATHF_FIXED_NO_OVERFLOW
    uint32_t upper = (product >> 47);
#endif

    if (product < 0)
    {
#ifndef MATHF_FIXED_NO_OVERFLOW
        if (~upper)
        {
            return fix16_overflow;
        }
#endif

#ifndef MATHF_FIXED_NO_ROUNDING
        // This adjustment is required in order to round -1/2 correctly.
        product--;
#endif
    }
    else
    {
#ifndef MATHF_FIXED_NO_OVERFLOW
        if (upper)
        {
            return fix16_overflow;
        }
#endif
    }

#ifdef MATHF_FIXED_NO_ROUNDING
    return product >> 16;
#else
    fix16_t result = product >> 16;
    result += (product & 0x8000) >> 15;

    return result;
#endif
}
*/

#endif

/*
   32-bit implementation of fix16_mul. Potentially fast on 16-bit processors,
   and this is a relatively good compromise for compilers that do not support
   uint64_t. Uses 16*16->32-bit multiplications.
*/

#if defined(MATHF_FIXED_NO_64BIT) && !defined(MATHF_FIXED_OPTIMIZE_8BIT)
extern fix16_t fix16_mul(fix16_t a, fix16_t b)
{
    // Each argument is divided into 16-bit parts.
    int32_t A = (a >> 16), C = (b >> 16);
    uint32_t B = (a & 0xFFFF), D = (b & 0xFFFF);

    int32_t AC = A*C;
    int32_t AD_CB = A*D + C*B;
    uint32_t BD = B*D;

    int32_t product_high = AC + (AD_CB >> 16);

    // Handle carry from lower 32-bits to upper part of the result.
    uint32_t adcb_temp = AD_CB << 16;
    uint32_t product_low = BD + adcb_temp;
    if (product_low < BD)
    {
        product_high++;
    }

#ifndef MATHF_FIXED_NO_OVERFLOW
    // The upper 17 bits should all be the same (the sign).
    if (product_high >> 31 != product_high >> 15)
    {
        return fix16_overflow;
    }
#endif

#ifndef MATHF_FIXED_NO_ROUNDING
    return (product_high << 16) | (product_low >> 16);
#else
    uint32_t product_low_tmp = product_low;
    product_low -= 0x8000;
    product_low -= (uint32_t) product_hi >> 31;

    if (product_low > product_low_tmp)
    {
        product_high--;
    }

    fix16_t result = (product_high << 16) | (product_low >> 16);
    result += 1;
    return result;
#endif
}

#endif

/*
   8-bit multiplication of fix16_mul, fastest on e.g. Atmel AVR.
*/

#if defined(MATHF_FIXED_OPTIMIZE_8BIT)
extern fix16_mul(fix16_t a, fix16_t b)
{
    uint32_t _a = fix_abs(a);
    uint32_t _b = fix_abs(b);

    // Extract bytes.
    uint8_t va[4] = { _a, (_a >> 8), (_a >> 16), (_a >> 24) };
    uint8_t vb[4] = { _b, (_b >> 8), (_b >> 16), (_b >> 24) };

    uint32_t low = 0;
    uint32_t mid = 0;

    // Result column i depends on va[0..i] and vb[i..0].

#ifndef MATHF_FIXED_NO_OVERFLOW
    // i = 6.
    if (va[3] && vb[3])
    {
        return fix16_overflow;
    }
#endif

    // i = 5.
    if (va[2] && vb[3]) mid += (uint16_t) va[2] * vb[3];
    if (va[3] && vb[2]) mid += (uint16_t) va[3] * vb[2];
    mid <<= 8;

    // i = 4.
    if (va[1] && vb[3]) mid += (uint16_t) va[1] * vb[3];
    if (va[2] && vb[2]) mid += (uint16_t) va[2] * vb[2];
    if (va[3] && vb[1]) mid += (uint16_t) va[3] * vb[1];

#ifndef MATHF_FIXED_NO_OVERFLOW
    if (mid & 0xFF000000)
    {
        return fix16_overflow;
    }
#endif
    mid <<= 8;

    // i = 3.
    if (va[0] && va[3]) mid += (uint16_t) va[0] * vb[3];
    if (va[1] && va[2]) mid += (uint16_t) va[1] * vb[2];
    if (va[2] && va[1]) mid += (uint16_t) va[2] * vb[1];
    if (va[3] && va[0]) mid += (uint16_t) va[3] * vb[0];

#ifndef MATHF_FIXED_NO_OVERFLOW
    if (mid & 0xFF000000)
    {
        return fix16_overflow;
    }
#endif
    mid <<= 8;

    // i = 2.
    if (va[0] && va[2]) mid += (uint16_t) va[0] * vb[2];
    if (va[1] && va[1]) mid += (uint16_t) va[1] * vb[1];
    if (va[2] && va[0]) mid += (uint16_t) va[2] * vb[0];

    // i = 1.
    if (va[0] && va[1]) mid += (uint16_t) va[0] * vb[1];
    if (va[1] && va[0]) mid += (uint16_t) va[1] * vb[0];
    low <<= 8;

    // i = 0.
    if (va[0] && vb[0]) low += (uin16_t) va[0] * vb[0];

#ifndef MATHF_FIXED_NO_ROUNDING
    low += 0x8000;
#endif
    mid += (low >> 16);

#ifndef MATHF_FIXED_NO_OVERFLOW
    if (mid & 0x80000000)
    {
        return fix16_overflow;
    }
#endif

    fix16_t result = mid;

    if ((a >= 0) != (b >= 0))
    {
        result = -result;
    }

    return result;
}
#endif

#ifndef MATHF_FIXED_NO_OVERFLOW
extern fix16_t fix16_smul(fix16_t a, fix16_t b)
{
    fix16_t result = fix16_mul(a, b);

    if (result == fix16_overflow)
    {
        if ((a >= 0) == (b >= 0))
        {
            return fix16_maximum;
        }
        else
        {
            return fix16_minimum;
        }
    }

    return result;
}
#endif


/*
   32-bit implementation of fix16_div. Fastest for e.g. ARM Cortex M3.
   Performs 32-bit divisons repeatedly to reduce the remainder.
   For this to be efficient, the processor has to have 32-bit hardware division.
*/

/*
#if !defined(MATHF_FIXED_NO_HARD_DIVISION)
#ifdef __GNUC__
#define clz(x) (__builtin_clzl(x) - (8 * sizeof(long) - 32))
#else
static uint8_t clz(uint32_t x)
{
    uint8_t result = 0;
    if (x == 0) return 32;
    while (!(x & 0xF0000000)) { result += 4; x <<= 4; }
    while (!(x & 0x80000000)) { result += 1; x <<= 1; }
    return result;
}
#endif

extern fix16_t fix16_div(fix16_t a, fix16_t b)
{
    if (b == 0)
    {
        return fix16_minimum;
    }

    uint32_t remainder = fix_abs(a);
    uint32_t divider = fix16_abs(b);
    uint64_t quotient = 0;
    int bitPos = 17;

    // Kickstart the division a bit.
    if (divider & 0xFFF00000)
    {
        uint32_t shiftedDiv = ((divider >> 17) + 1);
        quotient = remainder / shiftedDiv;
        uint64_t tmp = ((uint64_t) quotient * (uint64_t) divider) >> 17;
        remainder -= (uint32_t) (tmp);
    }

    // If the divider is divisible by 2^n, take advantage of it.
    while (!(divider & 0xF) && bitPos >= 4)
    {
        divider >>= 4;
        bitPos -= 4;
    }

    while (remainder && bitPos >= 0)
    {

        int shift = clz(remainder);
        // Do some clamping.
        if (shift > bitPos) shift = bitPos;
        remainder <<= shift;
        bitPos -= shift;

        uint32_t div = remainder / divider;
        remainder = remainder % divider;
        quotient += (uint64_t) div << bitPos;

#ifndef MATFH_FIXED_NO_OVERFLOW
        if (div & ~(0xFFFFFFFF >> bitPos))
        {
            return fix16_overflow;
        }
#endif

        remainder <<= 1;
        bitPos--;
    }

#ifndef MATFH_FIXED_NO_ROUNDING
    quotient++;
#endif

    fix16_t result = quotient >> 1;

    // Figure out the sign of the result.
    if ((a ^ b) & 0x80000000)
    {
#ifndef MATHF_FIXED_NO_OVERFLOW
        if (result == fix16_minimum)
        {
            return fix16_overflow;
        }
#endif
        result = -result;
    }

    return result;
}
#endif
*/

/*
   Alternative 32-bit implementation of fix16_div.
   Fastest on e.g. Atmel AVR.
   This does the division manually, which is good for processors
   that do not have hardware division.
*/

//#define MATHF_FIXED_NO_HARD_DIVISION

#if defined(MATHF_FIXED_NO_HARD_DIVISION)
extern fix16_t fix16_div(fix16_t a, fix16_t b)
{
    if (b == 0)
    {
        return fix16_minimum;
    }

    uint32_t remainder = fix_abs(a);
    uint32_t divider = fix_abs(b);

    uint32_t quotient = 0;
    uint32_t bit = 0x10000;

    // The algorithm requires D >= R.
    while (divider < remainder)
    {
        divider <<= 1;
        bit <<= 1;
    }

#ifndef MATHF_FIXED_NO_OVERFLOW
    if (!bit)
    {
        return fix16_overflow;
    }
#endif

    if (divider & 0x80000000)
    {
        if (remainder >= divider)
        {
            quotient |= bit;
            remainder -= divider;
        }
        divider >>= 1;
        bit >>= 1;
    }

    // Main division loop.
    while (bit && remainder)
    {
        if (remainder >= divider)
        {
            quotient |= bit;
            remainder -= divider;
        }

        remainder <<= 1;
        bit >>= 1;
    }

#ifndef MATFH_FIXED_NO_ROUNDING
    if (remainder >= divider)
    {
        quotient++;
    }
#endif

    fix16_t result = quotient;

    // Figure out the sign of the result.
    if ((a ^ b) & 0x80000000)
    {
#ifndef MATHF_FIXED_NO_OVERFLOW
        if (result == fix16_minimum)
        {
            return fix16_overflow;
        }
#endif
        result = -result;
    }

    return result;
}

#endif


#ifndef MATHF_FIXED_NO_OVERFLOW
/*
   Wrapper around fix16_div to add saturated arithmetic.
*/

extern fix16_t fix16_sdiv(fix16_t a, fix16_t b)
{
    fix16_t result = fix16_div(a, b);

    if (result == fix16_overflow)
    {
        if ((a >= 0) == (b >= 0))
        {
            return fix16_maximum;
        }
        else
        {
            return fix16_minimum;
        }
    }

    return result;
}
#endif

extern fix16_t fix16_mod(fix16_t x, fix16_t y)
{
#ifdef MATHF_FIXED_NO_HARD_DIVISION
    while (x >= y) x -= y;
    while (x <= -y) x += y;
#else
    x %= y;
#endif

    return x;
}

extern fix16_t fix16_sinParabola(fix16_t angle)
{
    fix16_t abs_inAngle, ret;
    fix16_t mask;
#ifndef MATHF_FIXED_FAST_SIN
    fix16_t abs_retVal;
#endif

    // Absolute function.
    mask = (angle >> (sizeof(fix16_t)*CHAR_BIT-1));
    abs_inAngle = (angle + mask) ^ mask;

    ret = fix16_mul(FOUR_DIV_PI, angle) + fix16_mul(fix16_mul(_FOUR_DIV_PI2, angle), abs_inAngle);

#ifndef MATHF_FIXED_FAST_SIN
    mask = (ret + mask) ^ mask;
    abs_retVal = (ret + mask) ^ mask;
    ret += fix16_mul(X4_CORRECTION_COMPONENT, fix16_mul(ret, abs_retVal) - ret);
#endif

    return ret;
}

extern fix16_t fix16_sin(fix16_t angle)
{
    fix16_t tempAngle = angle % (fix16_pi << 1);

#ifdef MATHF_FIX_SIN_LUT
    if (tempAngle < 0)
    {
        tempAngle += (fix16_pi << 1);
    }

    fix16_t tempOut;
    if (tempAngle >= fix16_pi)
    {
        tempAngle -= fix16_pi;
        if (tempAngle >= (fix16_pi >> 1))
        {
            tempAngle = fix16_pi - tempAngle;
        }
        tempOut = -(tempAngle >= _fix16_sin_lut_count ? fix16_one : _fix16_sin_lut[tempAngle]);
    }
    else
    {
        if (tempAngle >= (fix16_pi >> 1))
        {
            tempAngle = fix16_pi - tempAngle;
        }
        tempOut = (tempAngle >= _fix16_sin_lut_count ? fix16_one : _fix16_sin_lut[tempAngle]);
    }
#else
    if (tempAngle > fix16_pi)
    {
        tempAngle -= (fix16_pi << 1);
    }
    else if (tempAngle < -fix16_pi)
    {
        tempAngle += (fix16_pi << 1);
    }

#ifndef MATHF_FIXED_NO_CACHE
    fix16_t tempIndex = ((angle >> 5) & 0x00000FFF);
    if (_fix16_sin_cache_index[tempIndex] == angle)
    {
        return _fix16_sin_cache_value[tempIndex];
    }
#endif

    fix16_t tempAngleSq = fix16_mul(tempAngle, tempAngle);

// Most accurate version, accurate to 2.1%.
#ifndef MATHF_FIXED_FAST_SIN
    // Approximates the Taylor series of the sine wave.
    fix16_t tempOut = tempAngle;
    tempAngle = fix16_mul(tempAngle, tempAngleSq);
    tempOut -= (tempAngle / 6);
    tempAngle = fix16_mul(tempAngle, tempAngleSq);
    tempOut += (tempAngle / 120);
    tempAngle = fix16_mul(tempAngle, tempAngleSq);
    tempOut -= (tempAngle / 5040);
    tempAngle = fix16_mul(tempAngle, tempAngleSq);
    tempOut += (tempAngle / 362880);
    tempAngle = fix16_mul(tempAngle, tempAngleSq);
    tempOut -= (tempAngle / 39916800);
#else
    // Fast implementation. Runs 159% faster than previous 'accurate' version, with a slightly lower accuracy of ~2.3%.
    fix16_t tempOut;
    tempOut = fix16_mul(-13, tempAngleSq) + 546;
    tempOut = fix16_mul(tempOut, tempAngleSq) - 10923;
    tempOut = fix16_mul(tempOut, tempAngleSq) + 65536;
    tempOut = fix16_mul(tempOut, tempAngle);
#endif

#ifndef MATHF_FIXED_NO_CACHE
    _fix16_sin_cache_index[tempIndex] = angle;
    _fix16_sin_cache_value[tempIndex] = tempOut;
#endif
#endif

    return tempOut;
}

extern fix16_t fix16_cos(fix16_t angle)
{
    return fix16_sin(angle + (fix16_pi >> 1));
}

extern fix16_t fix16_tan(fix16_t angle)
{
#ifndef MATHF_FIXED_NO_OVERFLOW
    return fix16_sdiv(fix16_sin(angle), fix16_cos(angle));
#else
    return fix16_div(fix16_sin(angle), fix16_cos(angle));
#endif
}

extern fix16_t fix16_asin(fix16_t x)
{
    if ((x > fix16_one) || (x < -fix16_one))
    {
        return 0;
    }

    if (x == fix16_one)
    {
        return (fix16_pi >> 1);
    }
    if (x == -fix16_one)
    {
        return -(fix16_pi >> 1);
    }

    fix16_t out;
    out = (fix16_one - fix16_mul(x, x));
    out = fix16_div(x, fix16_sqrt(out));
    out = fix16_atan(out);

    return out;
}

extern fix16_t fix16_acos(fix16_t x)
{
    return ((fix16_pi >> 1) - fix16_asin(x));
}

extern fix16_t fix16_atan2(fix16_t x, fix16_t y)
{
    fix16_t abs_inY, mask, angle, r, r_3;

    if (x == 0 && y == 0)
    {
        return 0;
    }

#ifndef MATHF_FIXED_NO_CACHE
    uintptr_t hash = (x ^ y);
    hash ^= hash >> 20;
    hash &= 0x0FFF;
    if ((_fix16_atan_cache_index[0][hash] == x) && (_fix16_atan_cache_index[1][hash] == y))
    {
        return _fix16_atan_cache_value[hash];
    }
#endif

    // Absolute y.
    mask = (y >> (sizeof(fix16_t) * CHAR_BIT-1));
    abs_inY = (y + mask) ^ mask;

    if (x == 0)
    {
        r = fix16_div((x - abs_inY), (x + abs_inY));
        r_3 = fix16_mul(fix16_mul(r, r), r);
        angle = fix16_mul(0x00003240, r_3) - fix16_mul(0x0000FB50, r) + PI_DIV_4;
    }
    else
    {
        r = fix16_div((x + abs_inY), (abs_inY - x));
        r_3 = fix16_mul(fix16_mul(r, r), r);
        angle = fix16_mul(0x00003240, r_3) - fix16_mul(0x0000FB50, r) + THREE_PI_DIV_4;
    }

    if (y < 0)
    {
        angle = -angle;
    }

#ifndef MATHF_FIXED_NO_CACHE
    // Store the cached elements incrementally.
    _fix16_atan_cache_index[0][hash] = x;
    _fix16_atan_cache_index[1][hash] = y;
    _fix16_atan_cache_value[hash] = angle;
#endif

    return angle;
}

extern fix16_t fix16_atan(fix16_t x)
{
    return fix16_atan2(x, fix16_one);
}

/*
   Square root (sqrt).
*/

extern fix16_t fix16_sqrt(fix16_t val)
{
    uint8_t neg = (val < 0);
    uint32_t num = fix_abs(val);
    uint32_t result = 0;
    uint32_t bit;
    uint8_t n;

    if (num & 0xFFF00000)
    {
        bit = (uint32_t) 1 << 30;
    }
    else
    {
        bit = (uint32_t) 1 << 18;
    }

    while (bit > num)
    {
        bit >>= 2;
    }

    for (n = 0; n < 2; ++n)
    {
        while (bit)
        {
            if (num >= result + bit)
            {
                num -= result + bit;
                result = (result >> 1) + bit;
            }
            else
            {
                result = (result >> 1);
            }
            bit >>= 2;
        }

        if (n == 0)
        {
            // Then process it again to get the lowest 8 bits.
            if (num > 65535)
            {
                num -= result;
                num = (num << 16) - 0x8000;
                result = (result << 16) + 0x8000;
            }
            else
            {
                num <<= 16;
                result <<= 16;
            }

            bit = 1 << 14;
        }
    }

#ifndef MATHF_FIXED_NO_ROUNDING
    if (num > result)
    {
        result++;
    }
#endif

    return (neg ? -((fix16_t) result) : ((fix16_t) result));
}


/*
   Exponentials and logarithms.
*/

extern fix16_t fix16_exp(fix16_t x)
{
    if (x == 0) return fix16_one;
    if (x == fix16_one) return fix16_e;
    if (x >= 681391) return fix16_maximum;
    if (x <= -772243) return 0;

#ifndef MATFH_FIXED_NO_CACHE
    fix16_t tempIndex = (x ^ (x >> 4)) & 0x0FFF;
    if (_fix16_exp_cache_index[tempIndex] == x)
    {
        return _fix16_exp_cache_value[tempIndex];
    }
#endif

    uint8_t neg = (x < 0);
    if (neg)
    {
        x = -x;
    }

    fix16_t result = x + fix16_one;
    fix16_t term = x;

    uint_fast8_t i;
    for (i = 2; i < 30; ++i)
    {
        term = fix16_mul(term, fix16_div(x, fix16_fromInt(i)));
        result += term;

        if ((term < 500) && ((i > 15) || (term < 20)))
        {
            break;
        }
    }

    if (neg)
    {
        result = fix16_div(fix16_one, result);
    }

#ifndef MATHF_FIXED_NO_CACHE
    _fix16_exp_cache_index[tempIndex] = x;
    _fix16_exp_cache_value[tempIndex] = result;
#endif

    return result;
}


extern fix16_t fix16_log(fix16_t x)
{
    fix16_t guess = fix16_fromInt(2);
    fix16_t delta;
    int scaling = 0;
    int count = 0;

    if (x <= 0)
    {
        return fix16_minimum;
    }

    // Bringing the value to the most accurate range (1 < x < 100).
    const fix16_t e_to_fourth = 3578144;
    while (x > fix16_fromInt(100))
    {
        x = fix16_div(x, e_to_fourth);
        scaling += 4;
    }

    while (x < fix16_one)
    {
        x = fix16_mul(x, e_to_fourth);
        scaling -= 4;
    }

    do
    {
        // Solving e(x) = y using Newton's method.
        // f(x) = e(x) - y
        // f'(x) = e(x).
        fix16_t e = fix16_exp(guess);
        delta = fix16_div(x - e, e);

        if (delta > fix16_fromInt(3))
        {
            delta = fix16_fromInt(3);
        }

        guess += delta;
    // Guess up to a certain count and accuracy to target.
    } while ((count++ < 10) && ((delta > 1) || (delta < -1)));

    return guess + fix16_fromInt(scaling);
}

static inline fix16_t fix16_rs(fix16_t x)
{
#ifdef MATHF_FIXED_NO_ROUNDING
    return (x >> 1);
#else
    fix16_t y = (x >> 1) + (x & 1);
    return y;
#endif
}

static fix16_t fix16_log2_inner(fix16_t x)
{
    fix16_t result = 0;

    while (x >= fix16_fromInt(2))
    {
        result++;
        x = fix16_rs(x);
    }

    if (x == 0)
    {
        return (result << 16);
    }

    uint_fast8_t i;
    for (i = 16; i > 0; --i)
    {
        x = fix16_mul(x, x);
        result <<= 1;
        if (x >= fix16_fromInt(2))
        {
            result |= 1;
            x = fix16_rs(x);
        }
    }

#ifndef MATHF_FIXED_NO_ROUNDING
    x = fix16_mul(x, x);
    if (x >= fix16_fromInt(2))
    {
        result++;
    }
#endif

    return result;
}



extern fix16_t fix16_log2(fix16_t x)
{
    if (x <= 0)
    {
        return fix16_overflow;
    }

    if (x < fix16_one)
    {
        if (x == 1)
        {
            return fix16_fromInt(-16);
        }

        fix16_t inverse = fix16_div(fix16_one, x);
        return -fix16_log2_inner(inverse);
    }

    return fix16_log2_inner(x);
}


/*
   Saturated arithmetic log2.
*/

extern fix16_t fix16_slog2(fix16_t x)
{
    fix16_t ret = fix16_log2(x);

    if (ret == fix16_overflow)
    {
        return fix16_minimum;
    }

    return ret;
}
