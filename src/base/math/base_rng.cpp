#include "base_rng.h"

#include <math.h>

static THREAD_LOCAL RNG_state default_rng = { 0x853c49e6748fea9ULL, 0xda3e39cb94b95ULL };

extern void RNG_seedR(RNG_state* rng, u64 initState, u64 initSeq)
{
    /*
    if (rng == NULL)
    {
        return;
    }
    */

    rng->state = 0U;
    // Always generates an odd increment.
    rng->increment = (initSeq << 1) | 1;
    RNG_randR(rng);
    rng->state += initState;
    RNG_randR(rng);
}
extern void RNG_seed(u64 initState, u64 initSeq)
{
    RNG_seedR(&default_rng, initState, initSeq);
}


extern u32 RNG_randR(RNG_state* rng)
{
    /*
    if (rng == NULL)
    {
        return 0;
    }
    */

    u64 oldState = rng->state;
    rng->state = oldState * 6364136223846793005ULL + rng->increment;

    u32 xorshifted = (((oldState >> 18u) ^ oldState) >> 27u);
    u32 rot = oldState >> 59u;

    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

extern u32 RNG_rand(void)
{
    return RNG_randR(&default_rng);
}


extern u32 RNG_boundedRandR(RNG_state* rng, u32 bound)
{
    // To avoid bias, we need to make the range of the RNG a multiple of Bound,
    // which we do by dropping output less than a threshold.
    // A naive scheme to calculate the threshold would be to do:
    //
    // u32 threshold = (0x10000000ULL) % bound;
    //
    // but 64-bit div/mod is slower than 32-bit div/mod (especially on 32-bit platforms),
    // in essence we do
    //
    // u32 threshold = (0x10000000ULL-bound) % bound;
    //
    // because this version will calculate the same modulus,
    // but the LHS value is less than 2^32.

    u32 threshold = -bound % bound;

    // Uniformity guarantees that this loop will terminate.
    // In practice, it should usually terminate quickly;
    // on average (assuming all bounds are equally likely), 82.25% of the time
    // we can expect it to require just one iteration.
    // In the worst case scenario, someone passes a bound of 2^31 + 1
    // (i.e. 2147483649), which invalidates almost 50% of the range.
    // In practice, bounds are typically small and only using a tiny amount of the range
    // is eliminated.
    // Also, people use pow of two bounds, which allows the compiler
    // to optimize these modulo operations.
    for (;;)
    {
        u32 r = RNG_randR(rng);
        if (r >= threshold)
        {
            return r % bound;
        }
    }
}
extern u32 RNG_boundedRand(u32 bound)
{
    return RNG_boundedRandR(&default_rng, bound);
}

extern f32 RNG_randf32R(RNG_state* rng)
{
    // Collapses the 32-bit range of random numbers down to [0,1].
    return ldexpf((f32) RNG_randR(rng), -32);
}
extern f32 RNG_randf32(void)
{
    return RNG_randf32R(&default_rng);
}



extern inline f32 RNG_randf32_rangeR(RNG_state* rng, float mn, float mx)
{
    // Uses linear interpolation to scale the weight to [mn, mx].
    return mn + (RNG_randf32R(rng)) * (mx - mn);
}
extern inline f32 RNG_randf32_range(float mn, float mx)
{
    return RNG_randf32_rangeR(&default_rng, mn, mx);
}



extern inline int RNG_rand_rangeNUR(RNG_state* rng, int mn, int mx)
{
    return mn + (RNG_randR(rng) % (mx - mn));
}
extern inline int RNG_rand_rangeNU(int mn, int mx)
{
    return RNG_rand_rangeNUR(&default_rng, mn, mx);
}

// Uniform sampling.
extern inline int RNG_rand_rangeUniformR(RNG_state* rng, int mn, int mx)
{
    // Daniel Lemire's method.
    u64 res = (u64) RNG_randR(rng) * (u64) (mx - mn);
    return mn + (int) ((u32) (res >> 32));
}


extern inline int RNG_rand_rangeUniform(int mn, int mx)
{
    return RNG_rand_rangeUniformR(&default_rng, mn, mx);
}



// The modulo operator can generate non-uniform distributions if upper is not a pow of 2.
// The modulo operator becomes a fast & (upper - 1) operation in the compiled binary
// if upper is a power of 2.
// But Lemire's method is more predictable.

extern inline u32 RNG_rand_rangeIndexNUR(RNG_state* rng, u32 upper)
{
    return RNG_randR(rng) % upper;
}
extern inline u32 RNG_rand_rangeIndexNU(u32 upper)
{
    return RNG_rand_rangeIndexNUR(&default_rng, upper);
}

extern inline u32 RNG_rand_rangeIndexUniformR(RNG_state* rng, u32 upper)
{
    // Daniel Lemire's method of 64-bit scaling.
    u64 res = (u64) RNG_randR(rng) * (u64) upper;
    return (u32) (res >> 32);
}
extern inline u32 RNG_rand_rangeIndexUniform(u32 upper)
{
    return RNG_rand_rangeIndexUniformR(&default_rng, upper);
}






// Standard deviation normal curve distribution.
extern f32 RNG_stdNormR(RNG_state* rng)
{
    // Box-Muller transform.
    if (rng == NULL)
    {
        return 0;
    }

    static const f32 epsilon = 1e-6f;

    f32 u1 = epsilon;
    f32 u2 = 0.0f;

    do
    {
        u1 = (RNG_randf32R(rng)) * 2.0f - 1.0f;
    } while (u1 <= epsilon);
    u2 = (RNG_randf32R(rng)) * 2.0f - 1.0f;

    f32 mag = sqrtf(-2.0f * logf(u1));
    f32 z0 = mag * cosf(2.0f * 3.141592653f * u2);

    //f32 z1 = mag * sinf(PI2 * u2);

    return z0;
}
extern f32 RNG_stdNorm(void)
{
    return RNG_stdNormR(&default_rng);
}


// "Chances" are uniform.
extern int RNG_chanceR(RNG_state* rng, float chance)
{
    return RNG_randf32R(rng) <= chance;
}
extern int RNG_chance(float chance)
{
    return RNG_chanceR(&default_rng, chance);
}
