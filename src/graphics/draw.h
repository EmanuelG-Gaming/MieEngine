#ifndef DRAW_H_
#define DRAW_H_ 1

#include "../win/win.h"
//#include "../mem/arena.h"
#include "../base/math/mathf.h"

#include "texture.h"


/*
   Credits: Partially taken from https://github.com/LNooteBoom/RIEngine.
*/

/*
   Renderer:
   (original frontend by LNooteBoom, DirectX9 backend by Emanuel G)

   Features:
   - Draw pass system. Draw passes are basically "snapshots"
   of a camera (projection and view matrices).
   They can be switched by changing the index in a global Draw State.

   - The global Draw State would also have a Batched renderer:
      - It accumulates vertices and indices and "flushes" them
      to the screen when the state has changed,
      a new rendering method (i.e. meshes) comes in,
      or when the vertex capacity is reached.

      - Has immediate-style state setting
      (you check if the new state is not equal to the old state,
      and then you use graphics library calls if that's the case).
      - Has a Matrix Stack, so that the renderer can "remember" previous
      matrix transformations. Useful for things like skeletal animations.
      - Has a Pallette of 2 temporary colors, which can be set from a color mode enum
      (like COLOR_LR, COLOR_UD, COLOR_INOUT),
      and a bunch of other global state.
      This makes the actual function calls
      for drawing objects much simpler, like 'DrawBillboard(w, h)'.

   - Mid-level API.
   - You submit draw calls through either
   the batched renderer or through a Mesh.
   - 2 temporary target surfaces that can be
   filled using draw passes.
   - You create a "pipeline" by using an array of draw passes
   and you switch between them by changing the current pass index,
   in your drawing subroutine.

   - DirectX9 backend, but hopefully it would support more graphics APIs.
*/


#define DRAW_MAX_POINTLIGHTS 8
#define DRAW_MAX_TEX 8

#define DRAW_PASS_FLAG_3D 1
#define DRAW_PASS_FLAG_NORMAL_MATRIX 2
#define DRAW_PASS_FLAG_GAMMA 4
#define DRAW_PASS_FLAG_Z_BUFFER 8
#define DRAW_PASS_FLAG_SCENE_CONSTANTS 16


#define FBO_TEXTURE1_COLOR 0
#define FBO_TEXTURE1_DEPTH 1
#define FBO_TEXTURE2_COLOR 2
#define FBO_TEXTURE2_DEPTH 3


typedef enum SamplerMode {
    SAMPLER_NEAREST,
    SAMPLER_LINEAR,
} SamplerMode;

typedef enum CullingMode {
    CULL_NONE,
    CULL_BACK,
    CULL_FRONT,
} CullingMode;

typedef enum BlendMode {
    BLEND_ALPHA,
    BLEND_MULTIPLY,
    BLEND_ADD,
    BLEND_REPLACE,
    BLEND_SCREEN,
    BLEND_SUBTRACT,

    BLEND_COUNT,
} BlendMode;

// Used in the draw controller for
//specifying the color of shapes.

typedef enum ColorMode {
    COLOR1,
    COLOR2,
    COLOR_INOUT,
    COLOR_OUTIN,
    COLOR_LR,
    COLOR_UD,
} ColorMode;

typedef enum DepthStencilMode {
    DEPTH_STENCIL_DISABLE,
    DEPTH_STENCIL_DEPTH,
    DEPTH_STENCIL_NO_WRITE,
} DepthStencilMode;

typedef enum ProjectionMode {
    PROJECTION_IDENT,
    PROJECTION_PERSPECTIVE,
    PROJECTION_ORTHO,
    PROJECTION_CUSTOM,
} ProjectionMode;


// Custom vertex for mesh.
/*
typedef struct CustomVertex {
    float x, y, z, rhw;
    u32 color;
} CustomVertex;
*/

typedef struct GFX_mesh {
    void* user;

    void* vertexData;
    void* indexData;
    int nElements;
} GFX_mesh;



typedef struct DrawTex {
    GFX_texture* tex;
    float x, y, xs, ys;
} DrawTex;


typedef struct DrawProj_perspective {
    float fovy;
    float nr; // Near plane.
    float fr; // Far plane.
} DrawProj_perspective;

typedef struct DrawProj_ortho {
    float left, right;
    float top, bottom;
    float nr, fr;
} DrawProj_ortho;


// TODO: Move this somewhere else.
typedef struct CustomVertex {
    float x, y, z;//, rhw;
    //float rhw;
    u32 color;
    float u, v;
} CustomVertex;



typedef struct GFX_shader GFX_shader;

typedef struct DrawPass {
    b32 active;
    /*
    mat4* viewMatrix;
    vec3* camPosition;
    */

    float viewportX, viewportY;
    float viewportW, viewportH;

    // Stores states.
    SamplerMode samplerMode;
    CullingMode cullMode;
    DepthStencilMode depthStencilMode;

    ProjectionMode projectionMode;
    union {
        DrawProj_perspective perspective;
        DrawProj_ortho ortho;
        mat4* custom;
    } proj;

    mat4* viewMatrix;

    u32 fogColor;
    float fogMin, fogMax;

    int target;

    uint32_t flags;

    // Custom drawing method.
    void (*draw)(DrawPass* pass);
    void* user;
} DrawPass;

typedef struct DRAWSTATE {
    ARENA* drawArena;

    mat4 matStack[16];
    int matStackIdx;

    DrawTex tex[8];
    int nTex;

    mat4 normalMat;
    b32 normalMatValid;

    b32 uvModelMat;
    b32 wireframe;
    b32 cullInvert;

    GFX_shader* shader;

    float srcX, srcY, srcW, srcH;

    b32 hasBuffer;
    b32 zWrite;

    int currentPass;
    int nPasses;
    DrawPass* passes;

    BlendMode blend;
    ColorMode colorMode;

    float col1[4];
    float col2[4];

    int totalFlushes;
} DRAWSTATE;

extern DRAWSTATE drawState;

extern uint32_t fogColor;
extern float fogMin, fogMax;



extern int DrawInit(void);
extern void DrawTerminate(void);


/*
   Handle some state.
*/
extern int DrawMatIdentity(void);
extern int DrawMatTranslate3D(float x, float y, float z);
#define DrawMatTranslate(x, y) DrawMatTranslate3D(x, y, 0)

extern int DrawMatScale3D(float x, float y, float z);
#define DrawMatScale(x, y) DrawMatScale(x, y, 1)

extern int DrawMatRotateYA(float rad);
extern int DrawMatRotate3D(float x, float y, float z);

extern int DrawSetMatrix(float* mat);
extern int DrawMulMatrix(mat4* mat);

extern void DrawPushMat(void);
extern void DrawPopMat(void);

extern void DrawGetProjection(mat4* out);

/*
   State.
*/

extern void DrawBegin(void);
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

extern void DrawBlend(BlendMode blend);

extern void DrawColorMode(ColorMode mode);
extern void DrawColor(float r, float g, float b, float a);
extern void DrawColor2(float r, float g, float b, float a);

extern GFX_mesh* DrawUploadMesh(ARENA* arena, void* data, int nVertices);
extern void DrawMeshTerminate(GFX_mesh* mesh);
extern void DrawRenderMesh(GFX_mesh* mesh);

/*
   Vertices.
*/

extern void DrawPreFlush(int nverts, int nindices);
extern void DrawVertex3D(float x, float y, float z, float nx, float ny, float nz, float u, float v, float r, float g, float b, float a);

#define DrawVertex(x, y, z, u, v, r, g, b, a) \
    DrawVertex3D(x, y, z, 0, 0, 1, u, v, r, g, b, a)

extern void DrawIndices(int verts, int n, const u32* list);

/*
   Textures.
*/
extern void DrawSrcRect(float x, float y, float w, float h);
extern void DrawTextureOffsetScale(int slot, f32 x, f32 y, f32 xs, f32 ys);
extern void DrawTexture(int slot, GFX_texture* tex);


/*
   Draw passes.
*/

extern void DrawSetPipeline(int nPasses, DrawPass* passes);
extern void DrawSetPass2D(DrawPass* pass);
extern void DrawSetPass3D(DrawPass* passs);

extern int DrawFullFrame(void);



/************************
    Draw shapes.
*************************/

extern void DrawRect(f32 w, f32 h);
extern void DrawRectBillboard(f32 w, f32 h);
extern void DrawEllipse(int npoints, float w, float h);

extern void DrawArcSector(int npoints, f32 rStart, f32 r, f32 w1, f32 w2);

extern void DrawFillStar(int npoints, f32 r1, f32 r2);
extern void DrawLineStar(int npoints, f32 r1, f32 r2, f32 thickness);
extern void DrawLinePolygram(int npoints, int offset, f32 r1, f32 r2, f32 thickness);


extern void DrawSkybox(void);


extern int GraphicsInit(Window* window);
extern void GraphicsTerminate(void);

//extern int Draw_driverInit(void);

//extern void DrawClear(unsigned int color);
//extern void DrawPresent(void);

//extern void Draw_textString(const char* str, int x, int y, unsigned int color);

#endif /* DRAW_H_ */
