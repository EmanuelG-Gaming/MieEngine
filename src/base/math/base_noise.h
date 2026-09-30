#ifndef BASE_NOISE_H_
#define BASE_NOISE_H_ 1

#include "../base_defs.h"

typedef struct NoiseParams {
    float baseFrequency;
    float baseAmplitude;
    float lacunarity;
    float persistence;

    int octaves;
} NoiseParams;

/*
extern float NoisePerlin1D(float x);
extern float NoisePerlin2D(float x, float y);
*/

extern float NoisePerlinSimplex1D(float x);
extern float NoisePerlinSimplex2D(float x, float y);
extern float NoisePerlinSimplex3D(float x, float y, float z);

extern float NoiseSimplexFBM_1D(float x, NoiseParams params);
extern float NoiseSimplexFBM_2D(float x, float y, NoiseParams params);
extern float NoiseSimplexFBM_3D(float x, float y, float z, NoiseParams params);


#endif /* BASE_NOISE_H_ */
