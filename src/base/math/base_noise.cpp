#include "base_noise.h"

static inline i32 FastFloor(float fp)
{
    i32 i = static_cast<i32>(fp);
    return (fp < i) ? (i - 1) : (i);
}

// Permutation table.
static const u8 permutations[256] = {
    151, 160, 137, 91, 90, 15,
    131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
    190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
    88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
    77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
    102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
    135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
    5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
    223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
    129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
    251, 34, 242, 193, 238, 210, 144, 12, 191, 174, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
    49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
    138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
};

static inline u8 Hash(i32 i)
{
    return permutations[static_cast<u8>(i)];
}

static float gradient1D(i32 hash, float x)
{
    const i32 h = hash & 0x0F;
    float grad = 1.0f + (h & 7);
    if ((h & 8) != 0)
    {
        grad = -grad;
    }
    return (grad * x);
}

static float gradient2D(i32 hash, float x, float y)
{
    const i32 h = hash & 0x3F;
    const float u = h < 4 ? x : y;
    const float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f*v : 2.0f*v); // Computes dot product with (x, y).
}

static float gradient3D(i32 hash, float x, float y, float z)
{
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

extern float NoisePerlinSimplex1D(float x)
{
    float n0, n1;

    i32 i0 = FastFloor(x);
    i32 i1 = i0 + 1;

    // Distances to corners (between 0 and 1).
    float x0 = x - i0;
    float x1 = x0 - 1.0f;

    // Calculate the contribution from the 1st corner.
    float t0 = 1.0f - x0*x0;
    t0 *= t0;
    n0 = t0*t0 * gradient1D(Hash(i0), x0);

    // Calculate the contribution from the 2nd corner.
    float t1 = 1.0f - x1*x1;
    t1 *= t1;
    n1 = t1*t1 * gradient1D(Hash(i1), x1);

    // The maximum value of this noise is 8 * (3/4)^4 = 2.43125,
    // so scaling it by 0.395 scales to fit exactly within [-1,1].
    return 0.395f * (n0+n1);
}


extern float NoisePerlinSimplex2D(float x, float y)
{
    float n0, n1, n2;

    // Skewing/unskewing factors for 2D.
    static const float F2 = 0.366025403f;
    static const float G2 = 0.211324865f;

    // Skew the input space to determine which simplex cell we're in.
    const float s = (x + y) * F2;
    const float xs = x + s;
    const float ys = y + s;
    const i32 i = FastFloor(xs);
    const i32 j = FastFloor(ys);

    // Unskew the cell origin back to (x, y) space.
    const float t = static_cast<float>(i+j) * G2;
    const float X0 = i - t;
    const float Y0 = j - t;
    const float x0 = x - X0;
    const float y0 = y - Y0;

    // For the 2D case, the simplex shape is an equilateral triangle.
    // Determine which simplex we're in.
    i32 i1, j1;
    if (x0 > y0) // Lower triangle.
    {
        i1 = 1;
        j1 = 0;
    }
    else // Upper triangle
    {
        i1 = 0;
        j1 = 1;
    }

    const float x1 = x0 - i1 + G2;
    const float y1 = y0 - j1 + G2;
    const float x2 = x0 - 1.0f + 2.0f*G2;
    const float y2 = y0 - 1.0f + 2.0f*G2;

    // Work out the hashed gradient indices of the three simplex corners.
    const int gi0 = Hash(i + Hash(j));
    const int gi1 = Hash(i + i1 + Hash(j + j1));
    const int gi2 = Hash(i + 1 + Hash(j + 1));

    // Calculate the contribution from the first corner.
    float t0 = 0.5f - x0*x0 - y0*y0;
    if (t0 < 0.0f)
    {
        n0 = 0.0f;
    }
    else
    {
        t0 *= t0;
        n0 = t0 * t0 * gradient2D(gi0, x0, y0);
    }

    // Calculate the contribution from the 2nd corner.
    float t1 = 0.5f - x1*x1 - y1*y1;
    if (t1 < 0.0f)
    {
        n1 = 0.0f;
    }
    else
    {
        t1 *= t1;
        n1 = t1*t1 * gradient2D(gi1, x1, y1);
    }

    float t2 = 0.5f - x2*x2 - y2*y2;
    if (t2 < 0.0f)
    {
        n2 = 0.0f;
    }
    else
    {
        t2 *= t2;
        n2 = t2 * t2 * gradient2D(gi2, x2, y2);
    }

    return 45.23065f * (n0 + n1 + n2);
}

extern float NoisePerlinSimplex3D(float x, float y, float z)
{
    // Noise contributions from four corners.
    float n0, n1, n2, n3;

    // Skewing/unskewing factors for 2D.
    static const float F3 = 1.0f / 3.0f;
    static const float G3 = 1.0f / 6.0f;

    // Skew the input space to determine which simplex cell we're in.
    float s = (x + y + z) * F3;
    i32 i = FastFloor(x + s);
    i32 j = FastFloor(y + s);
    i32 k = FastFloor(z + s);

    // Unskew the cell origin back to (x, y) space.
    float t = (i+j+k) * G3;
    float X0 = i - t;
    float Y0 = j - t;
    float Z0 = k - t;
    float x0 = x - X0;
    float y0 = y - Y0;
    float z0 = z - Z0;


    // For the 2D case, the simplex shape is an equilateral triangle.
    // Determine which simplex we're in.
    i32 i1, j1, k1;
    i32 i2, j2, k2;
    if (x0 >= y0)
    {
        if (y0 >= z0)
        {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
        }
        else if (x0 >= z0)
        {
            i1 = 1; j1 = 0; k1 = 0;  i2 = 1; j2 = 0; k2 = 1;
        }
        else
        {
            i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1;
        }
    }
    else
    {
        if (y0 < z0)
        {
            i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1;
        }
        else if (x0 < z0)
        {
            i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1;
        }
        else
        {
            i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
        }
    }


    float x1 = x0 - i1 + G3;
    float y1 = y0 - j1 + G3;
    float z1 = z0 - k1 + G3;
    float x2 = x0 - i2 + 2.0f*G3;
    float y2 = y0 - j2 + 2.0f*G3;
    float z2 = z0 - k2 + 2.0f*G3;
    float x3 = x0 - 1.0f + 3.0f*G3;
    float y3 = y0 - 1.0f + 3.0f*G3;
    float z3 = z0 - 1.0f + 3.0f*G3;


    // Work out the hashed gradient indices of the three simplex corners.
    int gi0 = Hash(i + Hash(j + Hash(k)));
    int gi1 = Hash(i + i1 + Hash(j + j1 + Hash(k + k1)));
    int gi2 = Hash(i + i2 + Hash(j + j2 + Hash(k + k2)));
    int gi3 = Hash(i + 1 + Hash(j + 1 + Hash(k + 1)));

    // Calculate the contribution from the first corner.
    float t0 = 0.6f - x0*x0 - y0*y0 - z0*z0;
    if (t0 < 0.0f)
    {
        n0 = 0.0f;
    }
    else
    {
        t0 *= t0;
        n0 = t0 * t0 * gradient3D(gi0, x0, y0, z0);
    }

    // Calculate the contribution from the 2nd corner.
    float t1 = 0.6f - x1*x1 - y1*y1 - z1*z1;
    if (t1 < 0.0f)
    {
        n1 = 0.0f;
    }
    else
    {
        t1 *= t1;
        n1 = t1*t1 * gradient3D(gi1, x1, y1, z1);
    }

    float t2 = 0.6f - x2*x2 - y2*y2 - z2*z2;
    if (t2 < 0.0f)
    {
        n2 = 0.0f;
    }
    else
    {
        t2 *= t2;
        n2 = t2 * t2 * gradient3D(gi2, x2, y2, z2);
    }

    float t3 = 0.6f - x3*x3 - y3*y3 - z3*z3;
    if (t3 < 0.0f)
    {
        n3 = 0.0f;
    }
    else
    {
        t3 *= t3;
        n3 = t3 * t3 * gradient3D(gi3, x3, y3, z3);
    }



    return 32.0f * (n0 + n1 + n2 + n3);
}


/*
   Functions that make more complex noise.
*/

extern float NoiseSimplexFBM_1D(float x, NoiseParams params)
{
    float output = 0.0f;
    float denom = 0.0f;
    float frequency = params.baseFrequency;
    float amplitude = params.baseAmplitude;

    for (int i = 0; i < params.octaves; ++i)
    {
        output += (amplitude * NoisePerlinSimplex1D(x*frequency));
        denom += amplitude;

        frequency *= params.lacunarity;
        amplitude *= params.persistence;
    }

    return (output / denom);
}

extern float NoiseSimplexFBM_2D(float x, float y, NoiseParams params)
{
    float output = 0.0f;
    float denom = 0.0f;
    float frequency = params.baseFrequency;
    float amplitude = params.baseAmplitude;

    for (int i = 0; i < params.octaves; ++i)
    {
        output += (amplitude * NoisePerlinSimplex2D(x*frequency, y*frequency));
        denom += amplitude;

        frequency *= params.lacunarity;
        amplitude *= params.persistence;
    }

    return (output / denom);
}

extern float NoiseSimplexFBM_3D(float x, float y, float z, NoiseParams params)
{
    float output = 0.0f;
    float denom = 0.0f;
    float frequency = params.baseFrequency;
    float amplitude = params.baseAmplitude;

    for (int i = 0; i < params.octaves; ++i)
    {
        output += (amplitude * NoisePerlinSimplex3D(x*frequency, y*frequency, z*frequency));
        denom += amplitude;

        frequency *= params.lacunarity;
        amplitude *= params.persistence;
    }

    return (output / denom);
}

