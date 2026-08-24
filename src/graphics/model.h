#ifndef MODEL_H_
#define MODEL_H_ 1

#include "../base/mathf.h"

typedef struct VERTEX2 {
    vec2 pos;
    vec2 texcoords;
} VERTEX2;

typedef struct VERTEX3 {
    // The 3 basic building blocks for 3D.
    vec3 pos;
    vec3 normal;
    vec2 texcoords;
} VERTEX3;

typedef struct VTX {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
} VTX;
typedef struct VERTEX3COLOR {
    VTX vtx;
    float r, g, b, a;

    //vec3 pos;
    //vec3 normal;
    //vec3 col;
    //vec4 col;
    //vec2 texcoords;

    // Optional data.
    //vec4 col;
} VERTEX3COLOR;
//STATIC_GETSIZE(VTX);



typedef struct VERTEX3ANIM {
    vec3 pos;
    vec3 normal;
    vec2 texcoords;

    // Optional data.
    vec4 tangent;
    vec4 joint;
    vec4 weight;
    vec4 col;
} VERTEX3ANIM;

typedef VERTEX3COLOR vertex_t;
typedef int index_t;


typedef struct VERTEXBUFFER {
    int npos;
    int nnormal;
    int ntexcoords;
    int ntangents;
    int njoint;
    int nweight;
    int ncol;

    vec3* pos;
    vec3* normal;
    vec2* texcoords;
    vec4* tangents;
    vec4* joints;
    vec4* weights;
    vec3* col;
} VERTEXBUFFER;

typedef struct INDEXBUFFER {
    int npos;
    int nnormal;
    int ntexcoords;
    int ntangents;
    int njoint;
    int nweight;
    int ncol;

    int* pos;
    int* normal;
    int* texcoords;
    int* tangents;
    int* joints;
    int* weights;
    int* col;
} INDEXBUFFER;


#define MK_VERTEXBUF_BASIC(tpos, tnormal, ttexcoords) \
    CLITERAL(VERTEXBUFFER) { \
        .npos = ARRAY_SIZE(tpos), \
        .nnormal = ARRAY_SIZE(tnormal), \
        .ntexcoords = ARRAY_SIZE(ttexcoords), \
        \
        .pos = (tpos), \
        .normal = (tnormal), \
        .texcoords = (ttexcoords), \
        \
        .ntangents = njoint = nweight = ncol = 0, \
        .tangents = joints = weights = col = NULL, \
    } \

// Same thing, almost.
#define MK_INDEXBUF_BASIC(tpos, tnormal, ttexcoords) \
    CLITERAL(INDEXBUFFER) { \
        .npos = ARRAY_SIZE(tpos), \
        .nnormal = ARRAY_SIZE(tnormal), \
        .ntexcoords = ARRAY_SIZE(ttexcoords), \
        \
        .pos = (tpos), \
        .normal = (tnormal), \
        .texcoords = (ttexcoords), \
        \
        .ntangents = njoint = nweight = ncol = 0, \
        .tangents = joints = weights = col = NULL, \
    } \


typedef struct tagMESH {
    void* vertexBuffer;
    void* indexBuffer;

    vec3 center;
    i32 nfaces; // Triangles.
    i32 nvertices;

    // Fill these up.
    vertex_t* vertices;
    index_t* indices;
} MESH;


extern int MeshBuild(MESH* mesh,
    VERTEXBUFFER vtx,
    INDEXBUFFER idx, b32 dirty);

extern int MeshLoad_OBJ(MESH* mesh, const char* path);

extern void MeshTerminate(MESH* mesh);


#endif /* MODEL_H_ */
