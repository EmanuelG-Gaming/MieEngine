/*
    CHANGELOG:
    [8.7.2026] - Used direct setting rather than C functions,
    because C++ uses references (&) rather than C pointers.
*/

#ifndef MATHF_WRAP_H_
#define MATHF_WRAP_H_ 1


//#ifndef MATHF_H_ 
//#include "mathf.h"
//#endif

#include "mathf.h"

#ifdef __cplusplus

/*
   Vectors.
*/

/*
   3-dimensional vector.
*/

inline vec3::vec3()
{
    Vec3Zero(this);
}

inline vec3::vec3(float all)
{
    x = y = z = all;
}
inline vec3::vec3(float *v)
{
    x = v[0]; y = v[1]; z = v[2];
}

inline vec3::vec3(float xv, float yv)
{
    x = xv;
    y = yv;
    z = 0;
}
inline vec3::vec3(float xv, float yv, float zv)
{
    x = xv;
    y = yv;
    z = zv;
}
inline vec3::vec3(vec4 v)
{
    x = v.x;
    y = v.y;
    z = v.z;
}

inline vec3 vec3::Nor() const
{
    vec3 v;
    Vec3Nor3(&v, this);
    return v;
}

inline vec3 vec3::operator-() const
{
    vec3 v;
    Vec3MulScalar(&v, this, -1);
    return v;
}

inline vec3 vec3::operator+(const vec3& v2) const
{
    vec3 v;
    Vec3Add(&v, this, &v2);
    return v;
}
inline vec3 vec3::operator-(const vec3& v2) const
{
    vec3 v;
    Vec3Sub(&v, this, &v2);
    return v;
}

inline vec3 vec3::operator*(float s) const
{
    vec3 v;
    Vec3MulScalar(&v, this, s);
    return v;
}

inline vec3 vec3::operator*(const vec3& v2) const
{
    vec3 v;
    Vec3Mul(&v, this, &v2);
    return v;
}

inline vec3 vec3::operator/(float s) const
{
    vec3 v;
    Vec3DivScalar(&v, this, s);
    return v;
}


inline float vec3::Dot(const vec3& v1, const vec3& v2)
{
    return Vec3Dot3(&v1, &v2);
}
inline vec3 vec3::Cross(const vec3& v1, const vec3& v2)
{
    vec3 v;
    Vec3Cross3(&v, &v1, &v2);
    return v;
}

/*
   4-dimensional vector.
*/

inline vec4::vec4()
{
    x = y = z = w = 0;
}

inline vec4::vec4(float all)
{
    x = y = z = w = all;
}
inline vec4::vec4(float *v)
{
    x = v[0]; y = v[1]; z = v[2]; w = v[3];
}

inline vec4::vec4(float xv, float yv)
{
    x = xv;
    y = yv;
    z = 0;
    w = 0;
}
inline vec4::vec4(float xv, float yv, float zv)
{
    x = xv;
    y = yv;
    z = zv;
}

inline vec4::vec4(float xv, float yv, float zv, float wv)
{
    //fprintf(stderr, "calling constructor!\n");
    x = xv;
    y = yv;
    z = zv;
    w = wv;
}
inline vec4::vec4(vec4 vv, float wv) {
    Vec4Copy(this, &vv);
    this->w = wv;
}


inline vec4 vec4::Nor() const
{
    vec4 v;
    Vec4Nor4(&v, this);
    return v;
}
inline vec4 vec4::operator-() const
{
    vec4 v;
    Vec4MulScalar(&v, this, -1);
    return v;
}

inline vec4 vec4::operator+(const vec4& v2) const
{
    vec4 v;
    Vec4Add(&v, this, &v2);
    return v;
}
inline vec4 vec4::operator-(const vec4& v2) const
{
    vec4 v;
    Vec4Sub(&v, this, &v2);
    return v;
}

inline vec4 vec4::operator*(float s) const
{
    vec4 v;
    Vec4MulScalar(&v, this, s);
    return v;
}

inline vec4 vec4::operator*(const vec4& v2) const
{
    vec4 v;
    Vec4Mul(&v, this, &v2);
    return v;
}

inline vec4 vec4::operator/(float s) const
{
    vec4 v;
    Vec4DivScalar(&v, this, s);
    return v;
}


inline float vec4::Dot(const vec4& v1, const vec4& v2)
{
    return Vec4Dot3(&v1, &v2);
}
inline vec4 vec4::Cross(const vec4& v1, const vec4& v2)
{
    vec4 v;
    Vec4Cross3(&v, &v1, &v2);
    return v;
}

inline vec4 vec4::EulerAngles(float rx, float ry, float rz)
{
    // TODO: Maybe I'm wasting a bit of stack space, maybe.
    vec4 v = { rx, ry, rz, 0 };
    vec3 v3 = { rx, ry, rz };

    QuatEulerAngles_ZYX(&v, &v3);
    return v;
}

inline vec4 vec4::FromTrPos(struct TRANSFORM& tr)
{
    vec4 v;
    Vec4FromTrPos(&v, &tr);
    return v;
}

inline vec4 vec4::FromTrRot(struct TRANSFORM& tr)
{
    vec4 v;
    Vec4FromTrRot(&v, &tr);
    return v;
}

inline vec4 vec4::FromTrScl(struct TRANSFORM& tr)
{
    vec4 v;
    Vec4FromTrRot(&v, &tr);
    return v;
}

/*
   Matrices.
*/

inline mat4::mat4()
{
    Mat4Ident(this, 0.0f);
}
inline mat4::mat4(float ident)
{
    Mat4Ident(this, ident);
}

inline mat4::mat4(float *v)
{
    for (int i = 0; i < 16; ++i) c[i] = v[i];
}

inline mat4 mat4::operator*(const mat4 &m2) const
{
    mat4 mat;
    Mat4Mul(&mat, this, &m2);
    return mat;
}

inline vec4 mat4::operator*(const vec4 &v) const
{
    vec4 vec;
    Mat4MulVec4(&vec, this, &v);
    return vec;
}

inline mat4 mat4::Transpose() const
{
    mat4 mat;
    Mat4Transpose3(&mat, this);
    return mat;
}

inline void mat4::Translate(const vec4 &v)
{
    Mat4Translate(this, this, &v);
}

inline void mat4::Scale(const vec4 &v)
{
    Mat4Scale(this, this, &v);
}

inline void mat4::Rotate(const vec4 &v)
{
    Mat4Rotate(this, this, &v);
}

inline void mat4::Inverse3()
{
    Mat4Inverse3(this, this);
}

inline void mat4::TransposeSave3(float *dat)
{
    mat4 mat;
    Mat4Transpose3(&mat, this);
    for (int i = 0; i < 16; ++i) {
        c[i] = mat.c[i];
    }
}


/*
   Static functions.
*/

inline mat4 mat4::LookAt(const vec4 &eye, const vec4 &center, const vec4 &up)
{
    mat4 mat;
#ifdef MATHF_USE_LH
    Mat4LookAt_LH(&mat, &eye, &center, &up);
#else
    Mat4LookAt(&mat, &eye, &center, &up);
#endif
    return mat;
}
inline mat4 mat4::Look(const vec4 &eye, const vec4 &dir, const vec4 &up)
{
    mat4 mat;
#ifdef MATHF_USE_LH
    Mat4Look_LH(&mat, &eye, &dir, &up);
#else
    Mat4Look(&mat, &eye, &dir, &up);
#endif
    return mat;
}

inline mat4 mat4::FromRotation(const vec4 &r)
{
    mat4 mat;
    Mat4FromRotation(&mat, &r);
    return mat;
}
inline mat4 mat4::FromTranslation(const vec4 &v)
{
    mat4 mat;
    Mat4FromTranslation(&mat, &v);
    return mat;
}
inline mat4 mat4::FromScale(const vec4 &v)
{
    mat4 mat;
    Mat4FromScale(&mat, &v);
    return mat;
}

inline mat4 mat4::Perspective(float fovy, float aspect, float fnear, float ffar)
{
    mat4 mat;
#ifdef MATHF_USE_LH
    Mat4Perspective_LH(&mat, fovy, aspect, fnear, ffar);
#else
    Mat4Perspective(&mat, fovy, aspect, fnear, ffar);
#endif
    return mat;
}

inline mat4 mat4::Ortho(float left, float right, float bottom, float top, float fnear, float ffar)
{
    mat4 mat;
#ifdef MATHF_USE_LH
    Mat4Ortho_LH(&mat, left, right, bottom, top, fnear, ffar);
#else
    Mat4Ortho(&mat, left, right, bottom, top, fnear, ffar);
#endif
    return mat;
}

#endif // __cplusplus

#endif // MATHF_WRAP_H_
