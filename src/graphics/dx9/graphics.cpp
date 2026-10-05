#include "../draw.h"
#include "../../base/base_log.h"
#include "../../win/win.h"

/*
   I mean, there was also an utility library
   called <d3dx9.h> which allowed people
   to draw text more easily,
   but it's not included in modern Windows SDK.
*/

#include <d3d9.h>
#include <d3d9types.h>

#include <stdio.h>



#ifdef RELEASE
#define VBO_MAX_SIZE 0x4000
#define EBO_MAX_SIZE 0x4000
#else
#define VBO_MAX_SIZE 0x8000
#define EBO_MAX_SIZE 0x8000
#endif

#ifdef RELEASE
#define VERTEX_BUFFER_SIZE 0x4000
#define INDEX_BUFFER_SIZE 0x4000
#else
#define VERTEX_BUFFER_SIZE 0x8000
#define INDEX_BUFFER_SIZE 0x8000
#endif

/*
#define D3DCOLOR_ARGB(a, r, g, b) \
    ((DWORD) (((a) & 0xff) << 24) | (((r) & 0xff) << 16) | (((g) & 0xff) << 8) | (((b) & 0xff)))
*/


static LPDIRECT3D9 d3d = NULL;
static LPDIRECT3DDEVICE9 d3device = NULL;

static LPDIRECT3DSURFACE9 backSurf = NULL;

//static LPDIRECT3DSURFACE9 depthStencil2D = NULL;

static LPDIRECT3DVERTEXBUFFER9 streamVertexBuffer = NULL;
static LPDIRECT3DINDEXBUFFER9 streamIndexBuffer = NULL;

/*
   Render state.
*/
static int currNverts;
static int currNindices;

static void* streamVertexPdata = NULL;
static void* streamIndexPdata = NULL;

DRAWSTATE drawState;


static mat4 identMat = mat4{ 1.0f };

/*
   The surfaces.
*/
/*
static LPDIRECT3DSURFACE9 depthStencil2D = NULL;
static LPDIRECT3DSURFACE9 depthStencil3D = NULL;
static LPDIRECT3DSURFACE9 depthStencil3DnoWrite = NULL;
*/

/*
typedef struct TargetSurface {
    GFX_texture color;
    GFX_texture depthStencil;
} TargetSurface;
*/



// VBO for vertices.
//static LPDIRECT3DVERTEXBUFFER9 vbo = NULL;

/*
typedef struct CustomVertex {
    float x, y, z, rhw;
    u32 color;
} CustomVertex;
*/

typedef struct ConstantMats {
    mat4 modelView;
    mat4 projection;
    mat4 normMat;
} ConstantMats;



typedef struct TargetSurface {
    GFX_texture color;
    LPDIRECT3DSURFACE9 colorSurface;

    GFX_texture depthStencil;
    LPDIRECT3DSURFACE9 depthSurface;
} TargetSurface;


static TargetSurface targetSurface1 = { 0 };
static TargetSurface targetSurface2 = { 0 };

static LPDIRECT3DTEXTURE9 framebufferTex = NULL;
static LPDIRECT3DSURFACE9 framebufferSurface = NULL;



/*
typedef struct CustomVertex {
    float x, y, z, rhw;
    u32 color;
    float u, v;
} CustomVertex;
*/


// D3DFVF_XYZRHW is for already-transformed vertices
// when RHW is 1.
// Meanwhile, D3DFVF_XYZ is a specific case of D3DFVF_RHW,
// but with the RHW component set to 0.

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE| D3DFVF_TEX1)

// Gamma approximation.
static void DoGamma(float* r, float* g, float* b)
{
    *r = 0.75f * (*r * *r) + 0.25f * *r * (*r * *r);
    *g = 0.75f * (*g * *g) + 0.25f * *g * (*g * *g);
    *b = 0.75f * (*b * *b) + 0.25f * *b * (*b * *b);
}
// Int is between 0-255.
// 0.75f is converted into 3/4 of 255.
static void DoGammaInt(unsigned int* r, unsigned int* g, unsigned int* b)
{
    int vr = *r;
    int vg = *g;
    int vb = *b;
    *r = (int) ((3LL * 255LL * (vr * vr) + (long long) vr * (vr * vr)) / 260100LL);
    *g = (int) ((3LL * 255LL * (vg * vg) + (long long) vg * (vg * vg)) / 260100LL);
    *b = (int) ((3LL * 255LL * (vb * vb) + (long long) vb * (vb * vb)) / 260100LL);
}

static DWORD GetSrcBlend(BlendMode mode)
{
    switch (mode)
    {
        case BLEND_ALPHA: return D3DBLEND_SRCALPHA;
        case BLEND_MULTIPLY: return D3DBLEND_DESTCOLOR;
        case BLEND_ADD: return D3DBLEND_SRCALPHA;
        case BLEND_REPLACE: return D3DBLEND_ONE;
        case BLEND_SCREEN: return D3DBLEND_INVDESTCOLOR;
        case BLEND_SUBTRACT: return D3DBLEND_SRCALPHA;

        // Use alpha blending by default.
        default: return D3DBLEND_SRCALPHA;
    }
}
static DWORD GetDestBlend(BlendMode mode)
{
    switch (mode)
    {
        case BLEND_ALPHA: return D3DBLEND_INVSRCALPHA;
        case BLEND_MULTIPLY: return D3DBLEND_INVSRCALPHA;
        case BLEND_ADD: return D3DBLEND_ONE;
        case BLEND_REPLACE: return D3DBLEND_ZERO;
        case BLEND_SCREEN: return D3DBLEND_INVSRCCOLOR;
        case BLEND_SUBTRACT: return D3DBLEND_ONE;

        default: return D3DBLEND_INVSRCALPHA;
    }
}
static DWORD GetBlendOp(BlendMode mode)
{
    switch (mode)
    {
        case BLEND_SUBTRACT: return D3DBLENDOP_REVSUBTRACT;
        default: return D3DBLENDOP_ADD;
    }
}



extern void Draw_textString(const char* str, int x, int y, unsigned int color)
{
    HRESULT r = 0;

    if (!backSurf)
    {
        return;
    }

    HFONT hfont = (HFONT) GetStockObject(SYSTEM_FONT);

    // font = 0;
    //r = D3DXCreateFont();
}



/*
   Matrices. Scene data.
*/

static D3DMATRIX FillMatrix(mat4 a)
{
    D3DMATRIX r;
    // Transpose the matrix?
#if 1
    r._11 = a._11; r._12 = a._12; r._13 = a._13; r._14 = a._14;
    r._21 = a._21; r._22 = a._22; r._23 = a._23; r._24 = a._24;
    r._31 = a._31; r._32 = a._32; r._33 = a._33; r._34 = a._34;
    r._41 = a._41; r._42 = a._42; r._43 = a._43; r._44 = a._44;
#else
    r._11 = a._11; r._12 = a._21; r._13 = a._31; r._14 = a._41;
    r._21 = a._12; r._22 = a._22; r._23 = a._32; r._24 = a._42;
    r._31 = a._13; r._32 = a._23; r._33 = a._33; r._34 = a._43;
    r._41 = a._14; r._42 = a._24; r._43 = a._34; r._44 = a._44;

#endif
    return r;
}

static D3DMATRIX IdentMatrix(float ident)
{
    D3DMATRIX r;
    r._11 = ident; r._12 = 0; r._13 = 0; r._14 = 0;
    r._21 = 0; r._22 = ident; r._23 = 0; r._24 = 0;
    r._31 = 0; r._32 = 0; r._33 = ident; r._34 = 0;
    r._41 = 0; r._42 = 0; r._43 = 0; r._44 = ident;

    return r;
}

static void PrintMatrixD3D(D3DMATRIX mat)
{
    fprintf(stderr, "MAT4 D3D(\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f)\n",
    mat._11, mat._12, mat._13, mat._14,
    mat._21, mat._22, mat._23, mat._24,
    mat._31, mat._32, mat._33, mat._34,
    mat._41, mat._42, mat._43, mat._44);
}

static void SetNormalMat(void)
{
    drawState.normalMat = drawState.matStack[drawState.matStackIdx];
    drawState.normalMat.Inverse3();
    drawState.normalMat = drawState.normalMat.Transpose();
    drawState.normalMatValid = TRUE;
}

static void SetSceneConstants(void)
{
    DrawPass* pass = &drawState.passes[drawState.currentPass];

    // Set lights.
}

static void DrawSetConstants(mat4* model)
{
    DrawPass* pass = &drawState.passes[drawState.currentPass];

    /*
    if (model)
    {
        mats->modelView = *pass->viewMatrix * *model;
    }
    else
    {
        mats->modelView = *pass->viewMatrix;
    }
    */


    /*
    if (pass->flags & DRAW_PASS_FLAG_NORMAL_MATRIX)
    {
        mat4 m { mats->modelView };
        m.Inverse3();
        mats->normMat = m.Transpose();
    }
    else
    {
        mats->normMat = identMat;
    }
    */


    // Fill projection.
    mat4 proj;
    DrawGetProjection(&proj);

    D3DMATRIX m;
    //mat4 tmp {1.0f};
    //tmp.Scale({ 5.0f, 5.0f, 5.0f, 5.0f });
    //m = FillMatrix(tmp);

    if (model) {
        m = FillMatrix(*model);
        //PrintMat4(*model);
    } else {
        m = IdentMatrix(1.0f);
        //fprintf(stderr, "sus\n");
    }

    D3DMATRIX v = FillMatrix(*pass->viewMatrix);
    D3DMATRIX p = FillMatrix(proj);

    //PrintMatrixD3D(m);

    // Set up view matrix.
    /*
    HRESULT hr;
    hr = d3device->SetTransform(D3DTS_WORLD, &m);
    if (FAILED(hr))
    {
        // Debug the hresult error.
        const char* errorMsg = NULL;
        switch (hr)
        {
            case D3DERR_INVALIDCALL: errorMsg = "Invalid call (i.e. invalid parameters...)"; break;
            case D3DERR_NOTAVAILABLE: errorMsg = "Format or size not supported"; break;
            case D3DERR_OUTOFVIDEOMEMORY: errorMsg = "Out of GPU VRAM memory!"; break;
            case E_OUTOFMEMORY: errorMsg = "Out of system memory!"; break;
            case D3DERR_DEVICELOST: errorMsg = "Device lost!"; break;

            default: errorMsg = "Unknown error!"; break;
        }

        fprintf(stderr, "%s: Failed to load immutable texture! %s\n", __func__, errorMsg);
        return;
    }
    */

    d3device->SetTransform(D3DTS_WORLD, &m);
    d3device->SetTransform(D3DTS_VIEW, &v);
    d3device->SetTransform(D3DTS_PROJECTION, &p);
}


/*
   Vertices.
*/

static void DrawPrepare(void)
{
    if (!drawState.hasBuffer)
    {
        // We accumulate vertices,
        // by preparing (locking) the vertex and index buffers.
        size_t vertex_bytecount = VERTEX_BUFFER_SIZE * sizeof(CustomVertex);
        size_t index_bytecount = INDEX_BUFFER_SIZE * sizeof(u32);

        streamVertexBuffer->Lock(0, vertex_bytecount, (void **) &streamVertexPdata, D3DLOCK_DISCARD);
        streamIndexBuffer->Lock(0, index_bytecount, (void **) &streamIndexPdata, D3DLOCK_DISCARD);


        currNindices = 0;
        currNverts = 0;

        drawState.hasBuffer = TRUE;
    }
}

extern void DrawFlush(void)
{
    if (drawState.hasBuffer)
    {
        // Unmap.
        // We set the data to NULL (because the vertex pool location might be invalidated).
        streamVertexBuffer->Unlock();
        streamVertexPdata = NULL;
        streamIndexBuffer->Unlock();
        streamIndexPdata = NULL;

        // Reset model matrix.
        DrawSetConstants(NULL);

        d3device->SetStreamSource(0, streamVertexBuffer, 0, sizeof(CustomVertex));
        d3device->SetIndices(streamIndexBuffer);
        d3device->SetFVF(D3DFVF_CUSTOMVERTEX);

        // VERTEXED DRAWING.
        //d3device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, currNverts/ 3);

        // INDEXED DRAWING.
        d3device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, currNverts, 0, currNindices / 3);


        currNindices = 0;
        currNverts = 0;

        drawState.hasBuffer = FALSE;
    }
}

extern void DrawVertex3D(float x, float y, float z,
    float nx, float ny, float nz, float u, float v,
    float r, float g, float b, float a)
{
    DrawPrepare();

    int maxVerts = VBO_MAX_SIZE / sizeof(CustomVertex);
    if (currNverts >= maxVerts)
    {
        return;
    }

    vec4 normal { nx, ny, nz, 0 };
    DrawPass* pass = &drawState.passes[drawState.currentPass];
    if (pass->flags & DRAW_PASS_FLAG_GAMMA)
    {
        DoGamma(&r, &g, &b);
    }
    if (pass->flags & DRAW_PASS_FLAG_NORMAL_MATRIX)
    {
        if (!drawState.normalMatValid) {
            SetNormalMat();
        }
        normal = drawState.normalMat * normal;
    }

    // Homogeneous coordinates.
    vec4 pos = drawState.matStack[drawState.matStackIdx] * vec4 { x, y, z, 1 };

    CustomVertex* d = &((CustomVertex *) streamVertexPdata)[currNverts];

    d->x = pos.x;
    d->y = pos.y;
    d->z = pos.z;
    //d->rhw = 0.0f;

    // TODO: Add the rest of the vertex attributes.
    /*
    d->vtx.nx = normal.x;
    d->vtx.ny = normal.y;
    d->vtx.nz = normal.z;
    */

    if (drawState.uvModelMat && drawState.tex[0].tex)
    {
        d->u = pos.x / drawState.tex[0].tex->w + drawState.srcX;
        d->v = pos.y / drawState.tex[0].tex->h + drawState.srcY;

        if (drawState.srcH < 0) {
            d->v = 1 - d->v;
        }
    }
    else
    {
        d->u = u;
        d->v = v;
    }

    /*
    d->r = r;
    d->g = g;
    d->b = b;
    d->a = a;
    */

    // We encode color.
    // Format is 0xAARRGGBB
    // or 0xAABBGGRR
    /*
    d->color =
        (((u8)((int) (CLAMP(r, 0, 1) * 255.0f)))      ) |
        (((u8)((int) (CLAMP(g, 0, 1) * 255.0f))) << 8 ) |
        (((u8)((int) (CLAMP(b, 0, 1) * 255.0f))) << 16) |
        (((u8)((int) (CLAMP(a, 0, 1) * 255.0f))) << 24);
    */

    d->color = D3DCOLOR_ARGB(
        (BYTE) (a*255.0f),
        (BYTE) (r*255.0f),
        (BYTE) (g*255.0f),
        (BYTE) (b*255.0f)
    );

    //d->color = D3DCOLOR_RGBA(255, 255, 0, 0);

    //d->color = D3DCOLOR_ARGB(0, 255, 255, 255);



    currNverts += 1;
}

extern void DrawIndices(int verts, int n, unsigned int const* st)
{
    int maxindices = EBO_MAX_SIZE / sizeof(unsigned int);
    if (currNindices + n >= maxindices)
    {
        return;
    }
    int base = currNverts - verts;
    for (int i = 0; i < n; ++i)
    {
        ((int *) (streamIndexPdata))[currNindices++] = base + st[i];
    }
}

extern void DrawPreFlush(int nverts, int nindices)
{
    int maxVerts = VERTEX_BUFFER_SIZE / sizeof(CustomVertex);
    int maxIndices = INDEX_BUFFER_SIZE / sizeof(int);
    if (currNverts + nverts >= maxVerts ||
        currNindices + nindices >= maxIndices) {
        DrawFlush();
    }
}



/*
   Rendering stages (state handling).
*/

extern void DrawClear(u32 color)
{
    // We extract its components.
    unsigned int ccl[4] = {
        ((color >> 16) & 0xff),
        ((color >> 8 ) & 0xff),
        ((color      ) & 0xff),
        255,
    };

    if (drawState.passes[drawState.currentPass].flags & DRAW_PASS_FLAG_GAMMA)
    {
        DoGammaInt(&ccl[0], &ccl[1], &ccl[2]);
    }

    /*
    TargetSurfaec* target = NULL;
    if (drawState.passes[drawState.currentPass].target == 0)
    {
        target = &surface1;
    }
    else if (drawState.passes[drawState.currentPass].target == 1)
    {
        target = &surface2;
    }
    */

    // Target surface choosing.
    TargetSurface* target = NULL;
    if (drawState.passes[drawState.currentPass].target == 0)
    {
        target = &targetSurface1;
    }
    else if (drawState.passes[drawState.currentPass].target == 1)
    {
        target = &targetSurface2;
    }


    D3DCOLOR c = D3DCOLOR_XRGB(ccl[0], ccl[1], ccl[2]);
    if (target)
    {
        d3device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, c, 1.0f, 0);
    }
    else
    {
        // "Freestanding" mode.
        d3device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, c, 1.0f, 0);
    }
}

/*
extern void DrawPresent(void)
{
    // We don't present the framebuffer to anywhere,
    // so we just present it to the screen.
    d3device->Present(NULL, NULL, NULL, NULL);
}
*/

extern void DrawTextureOffsetScale(int slot, f32 x, f32 y, f32 xs, f32 ys)
{
    DrawTex* tex = &drawState.tex[slot];
    if (x == tex->x && y == tex->y && xs == tex->xs && ys == tex->ys)
    {
        return;
    }

    DrawFlush();
    tex->x = x;
    tex->y = y;
    tex->xs = xs;
    tex->ys = ys;
}

extern void DrawTexture(int slot, GFX_texture* tex)
{
    DrawTex* d = &drawState.tex[slot];
    if (slot >= drawState.nTex || d->tex != tex)
    {
        DrawFlush();

        if (slot >= drawState.nTex && tex)
        {
            drawState.nTex = slot + 1;
        }
        d->tex = tex;

        // We set the sampling mode.
        if (d->tex->flags & TEXTURE_POINT) {
            d3device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
            d3device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        } else {
            d3device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
            d3device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

            // Anisotropic rendering.
            /*
            d3device->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 2);
            d3device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
            d3device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
            */
        }

        // We set the texture.
        LPDIRECT3DTEXTURE9 drawTex = (LPDIRECT3DTEXTURE9) d->tex->handle;
        d3device->SetTexture(0, drawTex);
    }
}

extern GFX_texture* DrawGetFboTexture(int what)
{
    switch (what)
    {
        case FBO_TEXTURE1_COLOR:
            return &targetSurface1.color;
        case FBO_TEXTURE1_DEPTH:
            return &targetSurface1.depthStencil;
        case FBO_TEXTURE2_COLOR:
            return &targetSurface2.color;
        case FBO_TEXTURE2_DEPTH:
            return &targetSurface2.depthStencil;

        default:
            return NULL;
    }
}


extern void DrawBlend(BlendMode blend)
{
    if (drawState.blend != blend)
    {
        DrawFlush();
        drawState.blend = blend;

        d3device->SetRenderState(D3DRS_SRCBLEND, GetSrcBlend(drawState.blend));
        d3device->SetRenderState(D3DRS_DESTBLEND, GetDestBlend(drawState.blend));
    }
}
extern void DrawZBufferWrite(b32 flag)
{
    if (drawState.zWrite != flag)
    {
        DrawFlush();
        drawState.zWrite = flag;
    }
}
extern void DrawUVModelMat(b32 enable)
{
    if (drawState.uvModelMat != enable)
    {
        DrawFlush();
        drawState.uvModelMat = enable;
    }
}
extern void DrawCullInvert(b32 flag)
{
    if (drawState.cullInvert != flag &&
        drawState.passes[drawState.currentPass].cullMode != CULL_NONE)
    {
        DrawFlush();
        if (flag) {
            d3device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
        } else {
            d3device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
        }
    }
    drawState.cullInvert = flag;
}
extern void DrawWireframe(b32 flag)
{
    if (drawState.wireframe != flag)
    {
        DrawFlush();

        drawState.wireframe = flag;
    }
}


static int CreateTargetSurface(TargetSurface* surface, int width, int height)
{
    // Create some textures.
    HRESULT hr;
    LPDIRECT3DTEXTURE9 tex = NULL;

    hr = d3device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET,
        D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &tex, NULL);
    if (FAILED(hr))
    {
        LogErrorEmitF("%s: Failed to create render target texture!\n", __func__);
        return -1;
    }

    // Color texture.
    surface->color.handle = tex;
    surface->color.w = width;
    surface->color.h = height;
    surface->color.refs = 1;
    surface->color.flags = 0; // We use linear filtering here.

    hr = tex->GetSurfaceLevel(0, &surface->colorSurface);
    if (FAILED(hr))
    {
        LogErrorEmitF("%s: Failed to get surface of framebuffer!\n", __func__);
        return -1;
    }

    return 0;
}

extern void DrawSetTarget(void)
{
    DrawPass* pass = &drawState.passes[drawState.currentPass];

    // Set depth stencil mode.
    // TODO: Do.
    // We set the rendering target.

    if (pass->target == 0)
    {
        d3device->SetRenderTarget(1, targetSurface1.colorSurface);
    }
    else if (pass->target == 1)
    {
        d3device->SetRenderTarget(1, targetSurface2.colorSurface);
    }
    else
    {
        // Default target surface (we could add support for more surfaces).
        d3device->SetRenderTarget(1, NULL);
    }


    switch (pass->depthStencilMode)
    {
        case DEPTH_STENCIL_DISABLE:
            d3device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
            d3device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            break;
        case DEPTH_STENCIL_DEPTH:
            d3device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
            d3device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
            break;
        case DEPTH_STENCIL_NO_WRITE:
            // This is often used for transparent objects or special effects,
            // where the object is tested on the background, but it doesn't block
            // the other objects behind it.
            d3device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
            d3device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            break;
    }

    // Create the viewport.
    D3DVIEWPORT9 viewport = {
        (DWORD) pass->viewportX, (DWORD) pass->viewportY,
        (DWORD) pass->viewportW, (DWORD) pass->viewportH,
    };

    d3device->SetViewport(&viewport);

    // And then the culling mode.
    switch (pass->cullMode)
    {
        case CULL_NONE:
            d3device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
            break;
        case CULL_BACK:
            d3device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
            break;
        case CULL_FRONT:
            d3device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
            break;
    }
}

static void DeleteTargetSurface(TargetSurface* surface)
{
    // TODO: This might be an issue.
    TextureTerminate(surface, &surface->color);
    surface->colorSurface->Release();
}


extern void DrawBegin(void)
{
    d3device->BeginScene();
}

extern void DrawEnd(void)
{
    d3device->EndScene();

    d3device->Present(NULL, NULL, NULL, NULL);
}



extern GFX_mesh* DrawUploadMesh(ARENA* arena, void *data, void* indices, int nVertices)
{
    //GFX_mesh* mesh = ArenaPushStruct(arena, GFX_mesh);

    LPDIRECT3DVERTEXBUFFER9 vbo = NULL;

    size_t vertex_bytecount = nVertices*sizeof(CustomVertex);

    HRESULT res = d3device->CreateVertexBuffer(
        vertex_bytecount, 0,
        D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &vbo, NULL
    );

    if (FAILED(res))
    {
        LogErrorEmitF("%s: Failed to initialize mesh!\n", __func__);
        return NULL;
    }

    // And then we upload the data.
    void* pVertices;
    if (FAILED(vbo->Lock(0, vertex_bytecount, (void **) &pVertices, 0)))
    {
        LogErrorEmitF("%s: Failed to lock vertex buffer!\n", __func__);
        return NULL;
    }
    _MEMCPY(pVertices, data, vertex_bytecount);
    vbo->Unlock();

    GFX_mesh* mesh = ArenaPushStruct(arena, GFX_mesh);

    // Vertex mode.
    mesh->user = vbo;
    mesh->vertexData = data;
    mesh->indexData = NULL;
    // We use triangles, so we divide nVertices by 3.
    mesh->nElements = nVertices / 3;

    return mesh;
}


extern void DrawMeshTerminate(GFX_mesh* mesh)
{
    if (mesh == NULL)
    {
        return;
    }

    LPDIRECT3DVERTEXBUFFER9 vbo = (LPDIRECT3DVERTEXBUFFER9) mesh->user;
    if (vbo != NULL)
    {
        vbo->Release();
        // TODO: Maybe we're invalidating the entire user data.
        mesh->user = NULL;
    }
}


extern void DrawRenderMesh(GFX_mesh *mesh)
{
    // Use the matrix at the stack top
    // as the model matrix.
    DrawFlush();
    DrawSetConstants(&drawState.matStack[drawState.matStackIdx]);

    //fprintf(stderr, "%s: Stack index: %d\n", __func__, drawState.matStackIdx);


    LPDIRECT3DVERTEXBUFFER9 vbo = (LPDIRECT3DVERTEXBUFFER9) mesh->user;

    d3device->SetStreamSource(0, vbo, 0, sizeof(CustomVertex));
    d3device->SetFVF(D3DFVF_CUSTOMVERTEX);
    d3device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, mesh->nElements); // nElements
}



static HRESULT InitDirect3D(Window* win)
{
    LogInfoEmitF("%s: [Initializing DirectX9 backend...]", __func__);

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (d3d == NULL)
    {
        LogErrorEmitF("%s: Failed to acquire DirectX9 driver!\n", __func__);
        return E_FAIL;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    // Create the D3DDevice.
    //win->backend->
    HWND winHandle = *((HWND *) win->user);

    HRESULT res = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
        winHandle, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3device);
    if (FAILED(res)) {
        LogErrorEmitF("%s: Failed to initialize D3D9 device!\n", __func__);
        return res;
    }

    // We initialize the streamed buffers.
    d3device->CreateVertexBuffer(
        VERTEX_BUFFER_SIZE, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &streamVertexBuffer, NULL
    );
    d3device->CreateIndexBuffer(
        INDEX_BUFFER_SIZE, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        D3DFMT_INDEX32, D3DPOOL_DEFAULT, &streamIndexBuffer, NULL
    );

    // Set invalid blend mode (for changing purposes).
    drawState.blend = (BlendMode) (-1);


    // Create some render surfaces. (with window sizes)
    CreateTargetSurface(&targetSurface1, win->w, win->h);
    CreateTargetSurface(&targetSurface2, win->w, win->h);


    // Set some default states here.

    // Tells DirectX to grab diffuse/alpha values from the vertex color
    // rather than the material.
    d3device->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);

    // Set texture stages (for per-vertex alpha).
    d3device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    d3device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    d3device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

    // Use texture alpha and per-vertex diffuse alpha (Modulate multiplies them together).
    d3device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    d3device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    d3device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);



    // Ambient lighting (somehow the fixed-function pipeline never really leaved in D3D9).
    d3device->SetRenderState(D3DRS_AMBIENT, 0xffffffff);
    d3device->SetRenderState(D3DRS_LIGHTING, FALSE);

    // Enable alpha blending.
    d3device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

    // We use solid culling.
    d3device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    // Turn on the Z buffer.
    d3device->SetRenderState(D3DRS_ZENABLE, TRUE);
    // We use LEQ depth writing.
    d3device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    d3device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);


    // Set sampler.
    d3device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    d3device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);


    // Set some matrices
    /*
    {
        mat4 mat { 5.0f };
        mat.Scale({ 5.0f, 5.0f, 5.0f, 4.0f });

        PrintMat4(mat);

        D3DMATRIX m = FillMatrix(mat);
        d3device->SetTransform(D3DTS_WORLD, &m);
    }
    */

    return S_OK;
}

// Default draw pass array.
static DrawPass _passes[1] = { 0 };
static mat4 _viewMat { 1.0f };

extern int GraphicsInit(Window* win)
{
    // Create draw arena.
    drawState.drawArena = ArenaInit(MB(4), KB(4), ARENA_FLAG_GROWABLE);

    if (FAILED(InitDirect3D(win)))
    {
        LogErrorEmitF("%s: Failed to initialize drawing subroutine!\n", __func__);
        return -1;
    }

    DrawSetPipeline(1, _passes);
    DrawSetPass2D(&_passes[0]);
    _passes[0].active = TRUE;
    _passes[0].viewMatrix = &_viewMat;
    _passes[0].flags |= DRAW_PASS_FLAG_GAMMA;

    // Reset state.
    DrawReset();
    DrawSetTarget();

    drawState.currentPass = 0;

    // We don't load a shader (Yet) because we let DirectX9 handle the shaders
    // through the global state machine.

    return 0;
}

extern void GraphicsTerminate(void)
{
    // Release global structs.
    if (streamVertexBuffer != NULL)
    {
        streamVertexBuffer->Release();
        streamVertexBuffer = NULL;
    }
    if (streamIndexBuffer != NULL)
    {
        streamIndexBuffer->Release();
        streamIndexBuffer = NULL;
    }
    DeleteTargetSurface(&targetSurface1);
    DeleteTargetSurface(&targetSurface2);




    // Then the global device context.
    if (d3device != NULL)
    {
        d3device->Release();
        d3device = NULL;
    }

    if (d3d != NULL)
    {
        d3d->Release();
        d3d = NULL;
    }

    // And then the arena.
    ArenaTerminate(drawState.drawArena);
}


/*
   Texture.
*/


extern void AllocateMutableTexture(int width, int height, void** args, int flags)
{
    D3DFORMAT format = D3DFMT_X8R8G8B8;
    if (flags & TEXTURE_ALPHA)
    {
        format = D3DFMT_A8R8G8B8;
    }

    /*
       D3DPOOL_DEFAULT is for mutable textures,
       while D3DPOOL_MANAGED optimizes those textures if they're immutable.
    */
    d3device->CreateTexture(
        width, height, 1, 1, format,
        D3DPOOL_DEFAULT, reinterpret_cast<LPDIRECT3DTEXTURE9*>(args), NULL
    );
}

extern GFX_texture* LoadImmutableTextureFromPixels(void* alloc, int w, int h, unsigned char* pix, int flags)
{
    /*
    D3DCAPS9 caps;
    d3device->GetDeviceCaps(&caps);
    LogInfoEmitF("cap: w:%d, h:%d", caps.MaxTextureWidth, caps.MaxTextureHeight);
    */

    D3DFORMAT format = D3DFMT_X8B8G8R8;
    if (flags & TEXTURE_ALPHA)
    {
        format = D3DFMT_A8R8G8B8;
    }

    LPDIRECT3DTEXTURE9 texture2D = NULL;
    HRESULT hr = d3device->CreateTexture(w, h, 1, 0, format, D3DPOOL_MANAGED, &texture2D, NULL);
    if (FAILED(hr))
    {
        // Debug the hresult error.
        const char* errorMsg = NULL;
        switch (hr)
        {
            case D3DERR_INVALIDCALL: errorMsg = "Invalid call (i.e. invalid parameters...)"; break;
            case D3DERR_NOTAVAILABLE: errorMsg = "Format or size not supported"; break;
            case D3DERR_OUTOFVIDEOMEMORY: errorMsg = "Out of GPU VRAM memory!"; break;
            case E_OUTOFMEMORY: errorMsg = "Out of system memory!"; break;
            case D3DERR_DEVICELOST: errorMsg = "Device lost!"; break;

            default: errorMsg = "Unknown error!"; break;
        }

        LogErrorEmitF("%s: Failed to load immutable texture! %s\n", __func__, errorMsg);
        return NULL;
    }

    D3DLOCKED_RECT lockedRect;
    hr = texture2D->LockRect(0, &lockedRect, NULL, 0);
    if (FAILED(hr))
    {
        LogErrorEmitF("%s: Failed to lock rect!\n", __func__);
        return NULL;
    }

    // Copy row for row.
    BYTE* destBytes = static_cast<BYTE*>(lockedRect.pBits);
    const BYTE* srcBytes = static_cast<const BYTE *>(pix);
    UINT rowSize = w * 4; // 4-component RGBA.
    for (UINT y = 0; y < h; ++y)
    {
        _MEMCPY(destBytes + y * lockedRect.Pitch, srcBytes + y * lockedRect.Pitch, rowSize);
    }

    // Unlock texture.
    texture2D->UnlockRect(0);

    // Literally.
    GFX_texture* tex;
    if (alloc)
    {
        ARENA* arena = reinterpret_cast<ARENA *>(alloc);
        tex = ArenaPushStruct(arena, GFX_texture);
    }
    else
    {
        tex = (GFX_texture *) _MALLOC(sizeof(GFX_texture));
    }
    tex->w = w;
    tex->h = h;
    tex->flags = flags;
    tex->refs = 1;
    tex->handle = texture2D;
    tex->resourceView = NULL; // TODO: maybe?

    return tex;
}

extern void ReleaseTexture(GFX_texture* texture)
{
    if (texture == NULL)
    {
        return;
    }

    LPDIRECT3DTEXTURE9 tex2D = (LPDIRECT3DTEXTURE9) texture->handle;
    if (tex2D != NULL)
    {
        tex2D->Release();
    }

    texture->resourceView = NULL;
    texture->handle = NULL;
}

extern void TextureTerminate(void* alloc, GFX_texture* texture)
{
    if (texture == NULL)
    {
        return;
    }

    texture->refs -= 1;
    if (texture->refs > 0)
    {
        return;
    }

    ReleaseTexture(texture);

    if (alloc)
    {
        // You don't free individual stuff in an arena that easily.
    }
    else
    {
        _FREE(texture);
        texture = NULL;
    }
}

