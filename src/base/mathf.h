#ifndef MATHF_H_
#define MATHF_H_ 1

/*
   Compile - time configs.
*/

// TODO: Fix vector-matrix and matrix-matrix multiplication
// when not using SSE.
#define MATHF_USE_SSE 1

// Quake III arena Q_rsqrt()-like optimizations.
//#define MATHF_USE_FAST 1

#define MATHF_FORCE_INLINE 1
// Should C++ wrapper structs use
// left-handed coordinate space (like DirectX)?
#define MATHF_USE_LH 1

#define MATHF_F32_EPSILON (0.00001f)


#ifdef MATHF_USE_SSE
/* Intel Intrinsics full header. */
#    include <immintrin.h>
#define splat_x(r) _mm_shuffle_ps(r, r, 0)
#define splat_y(r) _mm_shuffle_ps(r, r, 0x55)
#define splat_z(r) _mm_shuffle_ps(r, r, 0xAA)
#define splat_w(r) _mm_shuffle_ps(r, r, 0xFF)
#endif




#ifdef MATHF_FORCE_INLINE
#define MATHF_IMPL _inline
#else
#define MATHF_IMPL
#endif

#include <stdint.h>
#include <stddef.h>

#include "base_defs.h"


// Float pi.
#define PI 3.14159265359
#define PI2 6.28318530718
#define TAU 6.28318530718

#define UINT16_T_MAX ((1 << 16) - 1)

/*
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(v, m, M) (MAX(MIN((v), (M)), (m))) 
*/




/* More numerically-stable Lerp. */
#define LERP(a, b, t) ((a) * (1 - (t)) + ((b) * (t)))
#define UNLERP(a, b, m) (((m) - (a)) / ((b) - (a)))


#define ABS(a) (((a) < 0) ? (-a) : (a))
#define SIGN(a) (((a) < 0) ? (-1) : (1))
#define RADIANS(a) ((a) * (PI / 180.0))
#define DEGREES(a) ((a) * (180.0 / PI))

#define ANGLE2C(r0, i0, angle) do { \
    (r0) = cosf(angle); \
    (i0) = sinf(angle); \
} while (0)

/* Multiply two complex numbers. */
#define CMUL(r0, i0, r1, i1, r2, i2) do { \
    float u = (r1) * (r2) - (i1) * (i2); \
    float a = (r1) * (i2) + (i1) * (r2); \
    (r0) = u; \
    (i0) = a; \
} while (0)

// Thresholded float equal.
#define EQUAL_F32(a, b) (ABS((a) - (b)) <= MATHF_F32_EPSILON)



/*
    UNFINISHED: Sin cache, cos cache.
*/

#define TRIG_CACHE_SIZE UINT16_T_MAX




// h
//#include <complex.h>

typedef float complex_t;

typedef struct COMPLEX {
    complex_t re, im;
} COMPLEX;


extern COMPLEX ComplexAdd(COMPLEX a, COMPLEX b);
extern COMPLEX ComplexSub(COMPLEX a, COMPLEX b);
extern COMPLEX ComplexExp(complex_t angle);


extern COMPLEX ComplexMul(COMPLEX a, COMPLEX b);
extern COMPLEX ComplexDiv(COMPLEX a, COMPLEX b);
extern COMPLEX ComplexConj(COMPLEX a);
extern complex_t ComplexLength(COMPLEX a);
extern complex_t ComplexLength2(COMPLEX a);
extern complex_t ComplexDst(COMPLEX a, COMPLEX b);
extern complex_t ComplexDst2(COMPLEX a, COMPLEX b);
extern COMPLEX ComplexScl(COMPLEX a, complex_t scl);

#define ComplexFmt(c) (float) (c).re, (float) (c).im

extern void FFT(COMPLEX* x, size_t count);
extern void InverseFFT(COMPLEX* x, size_t count);

extern void FFT2D(COMPLEX* x, size_t width, size_t height);
extern void InverseFFT2D(COMPLEX* x, size_t width, size_t height);


extern float g_sin_cache[TRIG_CACHE_SIZE];
extern float g_cos_cache[TRIG_CACHE_SIZE];

extern void TrigCache(void);


#define rRe rw
#define rIm rz

typedef struct TRANSFORM {
    float x, y, z;
    float rx, ry, rz, rw;
    float sx, sy, sz;
} TRANSFORM;


union vec4;

/*
    Where ALIGN_DECL(bytes) is alignas(bytes) in C++,
    or __declspec(align(bytes)) in MSVC,
    or __attribute(aligned(bytes)) in GCC/Clang.

    The latter two are known as compiler extensions.
    Alignment is used in SIMD, more particularly, in SSE,
    so that the variables can be correctly calculated by the computer's registers.
*/

typedef union ALIGN_DECL(16) vec2 {
    struct {
        float x, y;
    };
    struct {
        float u, v;
    };
    float ve[2];
} vec2;
#define VEC2(x, y) CLITERAL(VEC2) { x, y }

typedef union ALIGN_DECL(16) vec3 {
    struct {
        float x, y, z;
    };

    struct {
        float u, v, w;
    };
    struct {
        float r, g, b;
    };

    float ve[3];

#ifdef __cplusplus
    vec3();
    vec3(float all);
    vec3(float *v);
    vec3(float x, float y);
    vec3(float x, float y, float z);
    vec3(vec4 v);

    // Assignment operator.
    //vec3 operator=(const vec3& v2) const;

    vec3 Nor() const;
    vec3 operator-() const;

    vec3 operator+(const vec3& v2) const;
    vec3 operator-(const vec3& v2) const;

    vec3 operator*(float s) const;
    vec3 operator*(const vec3& v2) const;

    vec3 operator/(float s) const;


    static float Dot(const vec3& v1, const vec3& v2);
    static vec3 Cross(const vec3& v1, const vec3& v2);
#endif
} vec3;
#define VEC3(x, y, z) CLITERAL(vec3) { x, y, z }

typedef union ALIGN_DECL(16) vec4 {
    struct {
        float x, y, z, w;
    };
    struct {
        float r, g, b, a;
    };
    float v[4];
#ifdef __cplusplus
    vec4();
    vec4(float all);
    vec4(float *vv);
    vec4(float xv, float yv);
    vec4(float xv, float yv, float zv);
    vec4(float xv, float yv, float zv, float wv);
    vec4(vec4 vv, float wv);

    // Assignment operator.
    //vec4 operator=(const vec4& v2) const;

    vec4 Nor() const;
    vec4 operator-() const;

    vec4 operator+(const vec4& v2) const;
    vec4 operator-(const vec4& v2) const;

    vec4 operator*(float s) const;
    vec4 operator*(const vec4& v2) const;

    vec4 operator/(float s) const;


    static float Dot(const vec4& v1, const vec4& v2);
    static vec4 Cross(const vec4& v1, const vec4& v2);

    static vec4 EulerAngles(float rx, float ry, float rz);
    static vec4 FromTrPos(struct TRANSFORM& tr);
    static vec4 FromTrRot(struct TRANSFORM& tr);
    static vec4 FromTrScl(struct TRANSFORM& tr);
#endif
} vec4;


typedef vec4 quat;
#define VEC4(x, y, z, w) CLITERAL(vec4) { x, y, z, w }

typedef union ALIGN_DECL(16) mat3 {
    vec3 v[3];
    float m[3][3];
    float c[9];

#ifdef __cplusplus
    mat3();
    mat3(float ident);
    mat3(float *v);

    mat3 operator*(const mat3 &m2) const;
    vec3 operator*(const vec3 &v) const;

    mat3 Transpose() const;
    void Translate(const vec3 &v);
    void Scale(const vec3 &v);
    void Rotate(const vec3 &v);
    void Inverse3();
    void TransposeSave3(float *dat);

    static mat3 FromRotation(const vec3 &r);
    static mat3 FromTranslation(const vec3 &v);
    static mat3 FromScale(const vec3 &v);
#endif
} mat3;


typedef union ALIGN_DECL(16) mat4 {
    struct {
        float _11, _12, _13, _14;
        float _21, _22, _23, _24;
        float _31, _32, _33, _34;
        float _41, _42, _43, _44;
    };
    vec4 v[4];
    float m[4][4];
    float c[16];

#ifdef __cplusplus
    mat4();
	mat4(float ident);
	mat4(float *v);

	mat4 operator*(const mat4 &m2) const;
	vec4 operator*(const vec4 &v) const;

	mat4 Transpose() const;
	void Translate(const vec4 &v);
	void Scale(const vec4 &v);
	void Rotate(const vec4 &v);
	void Inverse3();
    void TransposeSave3(float *dat);

    static mat4 LookAt(const vec4 &eye, const vec4 &center, const vec4 &up);
    static mat4 Look(const vec4 &eye, const vec4 &dir, const vec4 &up);
    static mat4 FromRotation(const vec4 &r);
    static mat4 FromTranslation(const vec4 &v);
    static mat4 FromScale(const vec4 &v);

    static mat4 Perspective(float fovy, float aspect, float fnear, float ffar);
    static mat4 Ortho(float left, float right, float bottom, float top, float fnear, float ffar);
#endif
} mat4;


typedef struct tagRAY {
    vec3 origin;
    vec3 direction; // Normalized direction vector.
} RAY;

typedef struct tagRAYCASTRESULT {
    float t[4];
    i8 count;
    char padding[17];
} RAYCASTRESULT, RAYRESULT;

/*
    Low-level functions that can be optimized through SIMD
*/

/*
   2-dimensional vectors.
*/

extern void Vec2Zero(vec2* out);
extern void Vec2Copy(vec2* out, const vec2* v);
extern vec2* Vec2Add(vec2* out, const vec2* a, const vec2* b);
extern vec2* Vec2Sub(vec2* out, const vec2* a, const vec2* b);
extern vec2* Vec2Mul(vec2* out, const vec2* a, const vec2* b);
extern vec2* Vec2Div(vec2* out, const vec2* a, const vec2* b);

extern vec2* Vec2AddScalar(vec2* out, const vec2* a, float b);
extern vec2* Vec2SubScalar(vec2* out, const vec2* a, float s);
extern vec2* Vec2MulScalar(vec2* out, const vec2* a, float s);
extern vec2* Vec2DivScalar(vec2* out, const vec2* a, float s);

extern vec2* Vec2Lerp(vec2* out, const vec2* a, const vec2* b, float t);


/*
   3-dimensional vectors.
*/

extern void Vec3Zero(vec3* out);
extern void Vec3Copy(vec3* out, const vec3* v);
extern vec3* Vec3Add(vec3* out, const vec3* a, const vec3* b);
extern vec3* Vec3Sub(vec3* out, const vec3* a, const vec3* b);
extern vec3* Vec3Mul(vec3* out, const vec3* a, const vec3* b);
extern vec3* Vec3Div(vec3* out, const vec3* a, const vec3* b);

extern vec3* Vec3AddScalar(vec3* out, const vec3* a, float b);
extern vec3* Vec3SubScalar(vec3* out, const vec3* a, float s);
extern vec3* Vec3MulScalar(vec3* out, const vec3* a, float s);
extern vec3* Vec3DivScalar(vec3* out, const vec3* a, float s);

extern vec3* Vec3Lerp(vec3* out, const vec4* a, const vec4* b, float t);


/*
   4-dimensional vectors.
*/

extern void Vec4Zero(vec4* out);
extern void Vec4Copy(vec4* out, const vec4* v);
extern vec4* Vec4Add(vec4* out, const vec4* a, const vec4* b);
extern vec4* Vec4Sub(vec4* out, const vec4* a, const vec4* b);
extern vec4* Vec4Mul(vec4* out, const vec4* a, const vec4* b);
extern vec4* Vec4Div(vec4* out, const vec4* a, const vec4* b);

extern vec4* Vec4AddScalar(vec4* out, const vec4* a, float b);
extern vec4* Vec4SubScalar(vec4* out, const vec4* a, float s);
extern vec4* Vec4MulScalar(vec4* out, const vec4* a, float s);
extern vec4* Vec4DivScalar(vec4* out, const vec4* a, float s);

extern vec4* Vec4Lerp(vec4* out, const vec4* a, const vec4* b, float t);

// Transformation utilities.
extern vec4* Vec4FromTrPos(vec4* out, const TRANSFORM* tr);
extern vec4* Vec4FromTrRot(vec4* out, const TRANSFORM* tr);
extern vec4* Vec4FromTrScl(vec4* out, const TRANSFORM* tr);

/*
   3-rank square matrices.
*/

extern mat3* Mat3Copy(mat3* out, const mat3* a);

extern mat3* Mat3Mul(mat3* out, const mat3* a, const mat3* b);
extern vec3* Mat3MulVec3(vec3* out, const mat3* a, vec3* b);


/*
   4-rank square matrices.
*/

extern mat4* Mat4Copy(mat4* out, const mat4* a);

extern mat4* Mat4Mul(mat4* out, const mat4* a, const mat4* b);
extern vec4* Mat4MulVec4(vec4* out, const mat4* a, const vec4* b);
extern vec4* Mat4MulVec3(vec4* out, const mat4* a, vec4* b);

/*
   Front-end operations.
*/

/*
   2-dimensional vectors.
*/
extern vec2* Vec2Set2(vec2* out, float x, float y);
extern vec2* Vec2Set1(vec2* out, float x);
#define Vec2SetS(out, x) Vec2Set1(out, x)
#define Vec2Set(out, x, y) Vec3Set3(out, x, y)

extern vec2* Vec2SetV2(vec2* out, const vec2* src);
extern vec2* Vec2SetV1(vec2* out, const vec2* src);
#define Vec2SetVS(out, src) Vec2SetV1(out, src)
#define Vec2SetV(out, src) Vec2SetV2(out, src)

 
extern float Vec2Dst2(const vec2* v1, const vec2* v2);
extern float Vec2Dst2Squared(const vec2* v1, const vec2* v2);
extern vec2* Vec2Nor2(vec2* out, const vec2* v);
#define Vec2Dst(v1, v2) Vec2Dst2(v1, v2)
#define Vec2DstSquared(v1, v2) Vec2Dst2Squared(v1, v2)
#define Vec2Nor(out, v) Vec2Nor2(out, v)

extern float Vec2Dot2(const vec2* a, const vec2* b);
extern float Vec2Len2(const vec2* v);
extern float Vec2Len2Squared(const vec2* v);
#define Vec2Dot(a, b) Vec2Dot2(a, b)
#define Vec2Len(v) Vec2Len2(v)
#define Vec2LenSquared(v) Vec2Len2Squared(v)


extern vec2* Vec3Round2(vec2* out, const vec2* v);
#define Vec2Round(out, v) Vec2Round2(out, v)

extern float Vec2Cross(const vec2* a, const vec2* b);
// Finds the cross product between the triangle formed by the sides ab and ac.
extern float Vec2CrossTri(const vec2* a, const vec2* b, const vec2* c);


/*
   3-dimensional vectors.
*/

extern vec3* Vec3Set3(vec3* out, float x, float y, float z);
extern vec3* Vec3Set2(vec3* out, float x, float y);
extern vec3* Vec3Set1(vec3* out, float x);
#define Vec3SetS(out, x) Vec3Set1(out, x)
#define Vec3Set(out, x, y, z) Vec3Set3(out, x, y, z)

extern vec3* Vec3SetV3(vec3* out, const vec3* src);
extern vec3* Vec3SetV2(vec3* out, const vec3* src);
extern vec3* Vec3SetV1(vec3* out, const vec3* src);
#define Vec3SetVS(out, src) Vec3SetV1(out, src)
#define Vec3SetV(out, src) Vec3SetV3(out, src)

 
extern float Vec3Dst3(const vec3* v1, const vec3* v2);
extern float Vec3Dst3Squared(const vec3* v1, const vec3* v2);
extern vec3* Vec3Nor3(vec3* out, const vec3* v);
#define Vec3Dst(v1, v2) Vec3Dst3(v1, v2)
#define Vec3DstSquared(v1, v2) Vec3Dst3Squared(v1, v2)
#define Vec3Nor(out, v) Vec3Nor3(out, v)

extern float Vec3Dot3(const vec3* a, const vec3* b);
extern float Vec3Len3(const vec3* v);
extern float Vec3Len3Squared(const vec3* v);
#define Vec3Dot(a, b) Vec3Dot3(a, b)
#define Vec3Len(v) Vec3Len3(v)
#define Vec3LenSquared(v) Vec3Len3Squared(v)


extern vec3* Vec3Round3(vec3* out, const vec3* v);
#define Vec3Round(out, v) Vec3Round3(out, v)

extern vec3* Vec3Cross3(vec3* out, const vec3* a, const vec3* b);
extern vec3* Vec3RotateAxisAngle3(vec3* v, const vec3* axis, float angle);
extern float Vec3Angle3(const vec3* v1, const vec3* v2);
#define Vec3Cross(out, a, b) Vec3Cross3(out, a, b)
#define Vec3RotateAxisAngle(v, axis, angle) Vec3RotateAxisAngle3(v, axis, angle)

extern vec3* Vec3Outer(vec3* out, const vec3* a, const vec3* b);
extern float Vec3Inner(const vec3* v1, const vec3* v2);

extern vec3* Vec3Clamp(vec3* out, const vec3* a, const vec3* min, const vec3* max);
extern vec3* Vec3Min(vec3* out, const vec3* a, const vec3* b);
extern vec3* Vec3Max(vec3* out, const vec3* a, const vec3* b);

/*
   4-dimensional Vectors.
*/

extern vec4* Vec4Set4(vec4* out, float x, float y, float z, float w);
extern vec4* Vec4Set3(vec4* out, float x, float y, float z);
extern vec4* Vec4Set2(vec4* out, float x, float y);
extern vec4* Vec4Set1(vec4* out, float x);
#define Vec4Set(out, x) Vec4Set1(out, x)

extern vec4* Vec4SetV4(vec4* out, const vec4* src);
extern vec4* Vec4SetV3(vec4* out, const vec4* src);
extern vec4* Vec4SetV2(vec4* out, const vec4* src);
extern vec4* Vec4SetV1(vec4* out, const vec4* src);
#define Vec4SetV(out, x) Vec4SetV4

extern float Vec4Dot3(const vec4* a, const vec4* b);
#define Vec4Dot(a, b) Vec4Dot3(a, b)
extern float Vec4Len3(const vec4* v);
extern float Vec4Len3Squared(const vec4* v);
 
extern float Vec4Dst3(const vec4* v1, const vec4* v2);
extern float Vec4Dst3Squared(const vec4* v1, const vec4* v2);
extern vec4* Vec4Nor3(vec4* out, const vec4* v);


extern float Vec4Dot4(const vec4* a, const vec4* b);
extern float Vec4Len4(const vec4* v);
extern float Vec4Len4Squared(const vec4* v);

extern float Vec4Dst4(const vec4* v1, const vec4* v2);
extern float Vec4Dst4Squared(const vec4* v1, const vec4* v2);
extern vec4* Vec4Nor4(vec4* out, const vec4* v);

extern vec4* Vec4Round3(vec4* out, const vec4* v);
extern vec4* Vec4Round4(vec4* out, const vec4* v);

extern vec4* Vec4Cross3(vec4* out, const vec4* a, const vec4* b);
extern vec4* Vec4RotateAxisAngle3(vec4* v, const vec4* axis, float angle);
extern float Vec4Angle3(const vec4* v1, const vec4* v2);

extern vec4* Vec4Clamp(vec4* out, const vec4* a, const vec4* min, const vec4* max);
extern vec4* Vec4Min(vec4* out, const vec4* a, const vec4* b);
extern vec4* Vec4Max(vec4* out, const vec4* a, const vec4* b);


/*
   Matrices.
*/

extern void Mat4Load(mat4 *out, float data[16]);
extern mat4* Mat4Ident(mat4 *out, float ident);

extern mat4* Mat4FromTranslation(mat4 *out, const vec4 *a);
extern mat4* Mat4FromScale(mat4 *out, const vec4 *a);
extern mat4* Mat4FromRotation(mat4 *out, const vec4 *a);

extern mat4* Mat4Translate(mat4 *out, const mat4 *a, const vec4 *b);
extern mat4* Mat4Rotate(mat4 *out, const mat4 *a, const vec4 *b);
extern mat4* Mat4Scale3(mat4* out, const mat4* a, const vec4* b);
extern mat4* Mat4Scale4(mat4* out, const mat4* a, const vec4* b);
#define Mat4Scale(out, a, b) Mat4Scale3(out, a, b)

extern mat4 *Mat4Inverse3(mat4 *out, const mat4 *m);
extern mat4 *Mat4Transpose3(mat4 *out, const mat4 *m);
extern mat4 *Mat4Transpose4(mat4 *out, const mat4 *m);

extern mat4 *Mat4TransposePlace3(mat4 *out);
extern mat4 *Mat4TransposePlace4(mat4 *out);

/* The VP matrices. */
extern mat4 *Mat4LookAt(mat4 *out, const vec4 *eye, const vec4 *center, const vec4 *up);
extern mat4 *Mat4Look(mat4* out, const vec4* eye, const vec4* dir, const vec4* up);

extern mat4* Mat4Perspective(mat4 *out, float fovy, float aspect, float fnear, float ffar);
extern mat4* Mat4Ortho(mat4 *out, float left, float right, float bottom, float top, float fnear, float ffar);

// Left-handed variants.
extern mat4 *Mat4LookAt_LH(mat4 *out, const vec4 *eye, const vec4 *center, const vec4 *up);
extern mat4 *Mat4Look_LH(mat4* out, const vec4* eye, const vec4* dir, const vec4* up);

extern mat4* Mat4Perspective_LH(mat4 *out, float fovy, float aspect, float fnear, float ffar);
extern mat4* Mat4Ortho_LH(mat4 *out, float left, float right, float bottom, float top, float fnear, float ffar);



/* Quaternion rotation. */
extern vec4 *Mat4GetRotation(vec4 *out, const mat4 *mat);

extern void PrintVec2(vec2 v);
extern void PrintVec3(vec3 v);
extern void PrintVec4(vec4 v);

extern void PrintMat2(mat4 mat);
extern void PrintMat3(mat4 mat);
/* Print matrix with 4 vectors in a row-major order. */
extern void PrintMat4(mat4 mat);

/*
   Quaternions.
*/

// Right-handed quaternion convention.
extern quat* QuatMul(quat* out, const quat* a, const quat* b);
#define QuatNor(out, v) Vec4Nor4(out, v)

extern quat* QuatNormalLerp(quat* out, const quat* a, const quat* b, float t);
// Standard Tait-Bryan ZYX ordering.
extern quat* QuatEulerAngles_ZYX(quat* out, const vec3* angles);
#define QuatEulerAngles(out, angles) QuatEulerAngles_ZYX(out, angles)

extern quat* QuatSlerp(quat* out, const quat* a, const quat* b, float t);
extern quat* QuatAngleAxis(quat* out, const vec3* axis, float angle);
extern vec4* QuatRotate4(vec4* out, const quat* q, const vec4* v);
extern vec3* QuatRotate3(vec3* out, const quat* q, const vec3* v);

extern quat* QuatConjugate(quat* out, const quat* a);

/*
   Polynominal solvers.
*/

extern int SolveQuadratic(float sol[2], float a, float b, float c);
extern int SolveCubicNormed(float sol[3], float a2, float a1, float a0);
extern int SolveCubic(float sol[3], float a, float b, float c, float d);


/*
   Standard math functions.
*/

/* Integer part. */
extern f32 TruncF(f32 x);

extern f64 TruncD(f64 x);

extern f32 FloorF(f32 x);
extern f64 FloorD(f64 x);

extern f32 CeilF(f32 x);
extern f64 CeilD(f64 x);


extern int IsIntF(f32 x);
extern int IsIntD(f64 x);

extern f32 Fast_InvSqrt(f32 x);
extern f32 Fast_Inv(f32 x);
extern f32 Fast_Div(f32 a, f32 b);
extern f32 Fast_Sqrt(f32 x);

extern f32 InvSqrt(f32 x);
extern f32 Inv(f32 x);
extern f32 Div(f32 a, f32 b);
extern f32 Sqrt(f32 x);

extern f32 ModF(f32 a, f32 b);
extern f32 Sin(f32 angle);
extern f32 Cos(f32 angle);

extern i32 GetLog2(size_t n);
extern int IsPower2(size_t n);


/* Random number generation. */
extern u8 Rand_Byte(void);

/*
   Geometry.
*/

extern f32 GetVolumeSphere(f32 radius);
extern f32 GetVolumeBox(vec3 min, vec3 max);
extern f32 GetVolumeBoxSize(f32 w, f32 h, f32 d);
extern f32 GetVolumeCylinder(f32 radius, f32 height);
extern f32 GetVolumePrism(f32 radius, size_t sides, f32 height);

extern f32 GetAreaCircle(f32 radius);
extern f32 GetAreaSquare(f32 side);
extern f32 GetAreaRect(vec2 min, vec2 max);
extern f32 GetAreaRectSize(f32 w, f32 h);
extern f32 GetAreaRegular(size_t sides, f32 radius);
extern f32 GetAreaTriangle(vec3 a, vec3 b, vec3 c);
extern f32 GetAreaParallelogram(vec3 a, vec3 b, vec3 c);

extern f32 GetPerimCircle(f32 radius);
extern f32 GetPerimSquare(f32 side);
extern f32 GetPerimRect(vec2 min, vec2 max);
extern f32 GetPerimRectSize(f32 w, f32 h);
extern f32 GetPerimRegular(size_t sides, f32 radius);
extern f32 GetPerimTriangle(vec3 a, vec3 b, vec3 c);


/*
   Raycasting.
*/

extern RAYCASTRESULT RaycastPlaneDistance(RAY ray, float distance, vec3 normal);
extern RAYCASTRESULT RaycastPlane(RAY ray, vec3 center, vec3 normal);

extern RAYCASTRESULT RaycastSphere(RAY ray, vec3 center, float radius);
extern RAYCASTRESULT RaycastBox(RAY ray, vec3 min, vec3 max);

/*
   Geometry.
*/

extern vec3 GetLinePoint_MinPosition(vec3 point, vec3 linePos1, vec3 linePos2);
extern vec3 GetTrianglePoint_MinPosition(vec3 point, vec3 a, vec3 b, vec3 c);
extern vec3 GetSpherePoint_MinPosition(vec3 point, vec3 center, float radius);
extern vec3 GetSpherePoint_MaxPosition(vec3 point, vec3 center, float radius);

/*
   SDFs.
*/
extern float GetSDSphere_Squared(vec3 point, vec3 center, float radius);
extern float GetSDEllipsoid_Squared(vec3 point, vec3 center, vec3 sizes);


//#ifdef __cplusplus
//} /* extern "C" */
//#endif

//#include "mathf_wrap.h"

#endif /* MATHF_H_ */
