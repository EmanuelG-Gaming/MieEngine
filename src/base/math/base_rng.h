#ifndef BASE_RNG_H_
#define BASE_RNG_H_ 1

#include "../base_defs.h"

typedef struct RNG_state {
    u64 state;
    u64 increment;
} RNG_state;

extern void RNG_seedR(RNG_state* rng, u64 initState, u64 initSeq);
extern void RNG_seed(u64 initState, u64 initSeq);

extern u32 RNG_randR(RNG_state* rng);
extern u32 RNG_rand(void);

extern u32 RNG_boundedRandR(RNG_state* rng, u32 bound);
extern u32 RNG_boundedRand(u32 bound);

extern f32 RNG_randf32R(RNG_state* rng);
extern f32 RNG_randf32(void);

extern f32 RNG_randf32_rangeR(RNG_state* rng, float mn, float mx);
extern f32 RNG_randf32_range(float mn, float mx);


// Non-uniform range
extern int RNG_rand_rangeNUR(RNG_state* rng, int mn, int mx);
extern int RNG_rand_rangeNU(int mn, int mx);
// Cheap for pow of 2 upper range.
extern u32 RNG_rand_rangeIndexNUR(RNG_state* rng, u32 upper);
extern u32 RNG_rand_rangeIndexNU(u32 upper);

// Uniform range (Lemire's method).
extern int RNG_rand_rangeUniformR(RNG_state* rng, int mn, int mx);
extern int RNG_rand_rangeUniform(int mn, int mx);

extern u32 RNG_rand_rangeIndexUniformR(RNG_state* rng, u32 upper);
extern u32 RNG_rand_rangeIndexUniform(u32 upper);



// Standard deviation normal curve distribution.
extern f32 RNG_stdNormR(RNG_state* rng);
extern f32 RNG_stdNorm(void);

extern int RNG_chanceR(RNG_state* rng, float chance);
extern int RNG_chance(float chance);

#endif /* BASE_RNG_H_ */
