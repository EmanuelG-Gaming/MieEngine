#ifndef DRAW_3D_H_
#define DRAW_3D_H_ 1


#include "camera.h"
#include "texture.h"
#include "model.h"
#include "../misc/stack.h"
#include "../misc/pool.h"
#include "../misc/arena.h"


#define DRAW_MAX_POINTLIGHTS 8
#define DRAW_MAX_TEX 8

/*
typedef struct CAMERA3D {
    vec3 pos;
    vec3 dir;
    vec3 up;
} CAMERA3D;
*/

#define DRAW_PASS_FLAG_3D 1
#define DRAW_PASS_FLAG_NORMAL_MATRIX 2
#define DRAW_PASS_FLAG_GAMMA 4
#define DRAW_PASS_FLAG_Z_BUFFER 8
#define DRAW_PASS_FLAG_SCENE_CONSTANTS 16


#define FBO_TEXTURE1_COLOR 0
#define FBO_TEXTURE1_DEPTH 1
#define FBO_TEXTURE2_COLOR 2
#define FBO_TEXTURE2_DEPTH 3

typedef enum STDSHADERS {
    SHADER_NONE = 0,
    SHADER_2D,
    SHADER_3D,
} STDSHADERS;

typedef enum SAMPLERMODE {
    SAMPLER_NEAREST,
    SAMPLER_LINEAR,
} SAMPLERMODE;

typedef enum CULLINGMODE {
    CULL_NONE,
    CULL_BACK,
    CULL_FRONT,
} CULLINGMODE;

typedef enum BLENDMODE {
    //BLEND_NONE,
    BLEND_ALPHA,
    BLEND_MULTIPLY,
    BLEND_ADD,
    BLEND_REPLACE,
    BLEND_SCREEN,
    BLEND_SUBTRACT,

    BLEND_COUNT,
} BLENDMODE;

typedef enum COLORMODE {
    COLOR1,
    COLOR2,
    COLOR_INOUT,
    COLOR_OUTIN,
    COLOR_LR,
    COLOR_UD,
} COLORMODE;


typedef enum DEPTHSTENCILMODE {
    DEPTH_STENCIL_DISABLE,
    DEPTH_STENCIL_DEPTH,
    DEPTH_STENCIL_NO_WRITE,
} DEPTHSTENCILMODE;

typedef enum PROJMODE {
    PROJECTION_IDENT,
    PROJECTION_PERSPECTIVE,
    PROJECTION_ORTHO,
    PROJECTION_CUSTOM,
} PROJMODE;

typedef enum LINEPATH_FLAGS {
    LINE_PATH_NONE = 0,
    LINE_PATH_CLOSED = (1 << 0),
    LINE_PATH_JOINED = (1 << 1),
} LINEPATH_FLAGS;




typedef struct LINEPATH_CONTEXT
{
    // Arena that resets every frame.
    ARENA* frameArena;
    int nPaths;
    b32 initialized;
} LINEPATH_CONTEXT;

typedef struct LINEPATH {
    /*
    vec3* points;
    u32* flags;
    int* ids;
    int* freeList;

    int nPoints;
    int freeCount;
    int capacity;
    */
    vec2* points;
    int nPoints;
    b32 allocated;
    POOL pool;
} LINEPATH;

extern int LinePathInit(LINEPATH* path);
extern void LinePathTerminate(LINEPATH* path);

/*
extern int LinePathReserve(LINEPATH* path, size_t desired);

extern int LinePathAlloc(LINEPATH* path);
extern void LinePathDealloc(LINEPATH* path, int index);
*/

extern vec2* LinePathAdd(LINEPATH* path, float x, float y);
extern void LinePathSub(LINEPATH* path, vec2* ptr);

//extern void LinePathPushStar(LINEPATH* path, int points, float rotation, float r1, float r2);

extern void LinePathCreateStar(float* path, int pathLen, int points, float rotation, float r1, float r2);



// float* weights;


typedef struct LIGHT {
    float x, y, z;
    float ambR, ambG, ambB;
    float difR, difG, difB;
    float spcR, spcG, spcB;

    float constant;
    float linear;
    float quadratic;
    float intensity;
} LIGHT;

typedef struct DRAWPROJ_PERSPECTIVE {
    float fovy;
    float nr;
    float fr;
} DRAWPROJ_PERSPECTIVE;


typedef struct DRAWPROJ_ORTHO {
    float left, right;
    float top, bottom;
    float nr, fr;
} DRAWPROJ_ORTHO;

typedef struct DRAWTEX {
    TEXTURE* tex;
    float x, y, xs, ys;
} DRAWTEX;


typedef struct SHADER SHADER;

// Mostly references things.
typedef struct DRAWPASS {
    b32 active;
    mat4* viewMatrix;
    vec3* camPosition;

    float viewportX, viewportY;
    float viewportW, viewportH;

    STDSHADERS stdShader;

    SAMPLERMODE samplerMode;
    CULLINGMODE cullMode;
    DEPTHSTENCILMODE depthStencilMode;
    //BLENDMODE blendMode;
    //COLORMODE colorMode;

    // Tagged union.
    PROJMODE projectionMode;
    union {
        DRAWPROJ_PERSPECTIVE perspective;
        DRAWPROJ_ORTHO ortho;
        mat4* custom;
    } proj;

    LIGHT* dirLight;
    LIGHT* pointLights;

    uint32_t fogColor;
    float fogMin, fogMax;

    int target;

    uint32_t flags;

    void (*draw)(DRAWPASS* pass);
    void* user;
} DRAWPASS;

typedef struct DRAWSTATE {
    //STACKALLOC stack;

    mat4 matStack[16];
    int matStackIdx;

    DRAWTEX tex[8];
    int nTex;

    //mat4* uvModelMat;
    mat4 normalMat;
    b32 normalMatValid;
    b32 uvModelMat;
    b32 wireframe;
    b32 cullInvert;

    SHADER* shader;

    float srcX, srcY, srcW, srcH;

    b32 hasBuffer;
    b32 zWrite;

    int currentPass;
    int nPasses;
    DRAWPASS* passes;

    BLENDMODE blend;
    COLORMODE colorMode;

    float col1[4];
    float col2[4];

    int totalFlushes;

} DRAWSTATE;



extern CAMERA3D camera3Handle;
extern DRAWSTATE drawState;

extern LIGHT dirLight;
// extern LIGHT pointLights[8];

extern uint32_t fogColor;
extern float fogMin, fogMax;

extern int DrawInit(void);
extern void DrawTerminate(void);

/*
   Matrices.
*/
extern int DrawMatIdentity(void);
extern int DrawMatTranslate3D(float x, float y, float z);
#define DrawMatTranslate(x, y) DrawMatTranslate3D(x, y, 0)

extern int DrawSetMatrix(float* mat);


extern void DrawPushMat(void);
extern void DrawPopMat(void);

extern void DrawGetProjection(mat4* out);


/*
   State.
*/
extern void DrawBegin(void);
extern void DrawGfxEnd(void);
extern void DrawEnd(void);

extern void DrawIndices(int verts, int n, unsigned int const* st);
extern void DrawPreFlush(int nverts, int nindices);
extern void DrawFlush(void);

extern void DrawReset(void);
extern void DrawClear(uint32_t color);

extern void DrawSetTarget(void);

// Used by other files.
extern void DrawZBufferWrite(b32 flag);
extern void DrawUVModelMat(b32 enable);
extern void DrawWireframe(b32 flag);
extern void DrawCullInvert(b32 flag);
extern void DrawBlend(BLENDMODE blend);


extern void DrawColorMode(COLORMODE mode);
extern void DrawColor(float r, float g, float b, float a);
extern void DrawColor2(float r, float g, float b, float a);


extern int UploadMesh(MESH* m, void* verts, void* indices);
extern void ReleaseMesh(MESH* m);
extern int DrawMesh(MESH* m);

/*
   Vertices.
*/

extern void DrawPreFlush(int nverts, int nindices);
extern void DrawVertex3D(float x, float y, float z,
    float nx, float ny, float nz, float u, float v,
    float r, float g, float b, float a);
#define DrawVertex(x, y, z, u, v, r, g, b, a) \
    DrawVertex3D(x, y, z, 0, 0, 1, u, v, r, g, b, a)

extern void DrawIndices(int verts, int n, const u32* list);

/*
   Shapes.
*/

extern void DrawSrcRect(float x, float y, float w, float h);

extern void DrawRect(f32 w, f32 h);
extern void DrawRectBillboard(f32 w, f32 h);
extern void DrawEllipse(int npoints, f32 w, f32 h);
extern void DrawArcSector(int npoints, f32 rStart, f32 r, f32 w1, f32 w2);

extern void DrawLine(float x0, float y0, float x1, float y1, float thickness);

extern void DrawLinePathEx(float* pointData, int nPoints, float thickness, u32 flags);
extern void DrawLinePath(LINEPATH* path, float thickness, u32 flags);

/*
   Textures.
*/
extern void DrawTextureOffsetScale(int slot, f32 x, f32 y, f32 xs, f32 ys);
extern void DrawTexture(int slot, TEXTURE* tex);

/*
   Lights.
*/

extern void DrawClearLights(void);
extern void SetLight(LIGHT *l, f32 x, f32 y, f32 z,
    u32 color, f32 ambient, f32 diffuse, f32 specular,
    f32 linear, f32 quadratic, f32 intensity);


/*
   Draw passes.
*/
extern int DrawSetPipeline(int nPasses, DRAWPASS* passes);
extern int DrawSetPass2D(DRAWPASS* pass);
extern int DrawSetPass3D(DRAWPASS* pass);

extern int DrawFullFrame(void);


#endif /* DRAW_3D_H_ */
