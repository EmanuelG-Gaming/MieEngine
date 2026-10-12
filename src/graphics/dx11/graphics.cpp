#include "../../base/base_defs.h"
#include "../../base/math/mathf.h"

//#include "draw3d.h"
//#include "model.h"
//#include "texture.h"
//#include "font.h"

#include "../draw.h"
#include "../../io/asset.h"


#include <stdio.h>
#include <stdlib.h>

#include <map>

#include "../../win/win32/window.cpp"

#include <d3d11.h>
//#include <d3dcompiler.h>

// Ok.
//#define FULLSCREEN 0
//#define VSYNC_ENABLED 1
#define SCREEN_DEPTH (1000.0f)
#define SCREEN_NEAR (0.3f)


#if defined(UNICODE) || defined(_UNICODE)
#define WIN_HAS_UNICODE 1
#endif

#ifdef WIN_HAS_UNICODE
#define UNICODE_CAST(str) (const wchar_t *) str
#else
#define UNICODE_CAST(str) (const char_t *) str
#endif

#ifdef RELEASE 
#define VBO_MAX_SIZE 0x4000
#define EBO_MAX_SIZE 0x4000
#else
#define VBO_MAX_SIZE 0x8000
#define EBO_MAX_SIZE 0x8000
#endif


#ifdef RELEASE 
#define VERTEX_BUFFER_sIZE 0x4000
#define INDEX_BUFFER_SIZE 0x4000
#else
#define VERTEX_BUFFER_SIZE 0x8000
#define INDEX_BUFFER_SIZE 0x8000
#endif

/*
   DirectX Constant buffers.
*/


typedef struct ConstantVs {
    mat4 modelView;
    mat4 projection;
    mat4 normMat;
} ConstantVs;

typedef struct ConstantPsTex {
    float offs[2];
    float scale[2];
} ConstantPsTex;

typedef struct ConstantPsLight {
    float position[4];
    float ambient[4];
    float diffuse[4];
    float specular[4];

    float constant;
    float linear;
    float quadratic;
    float intensity;
} ConstantPsLight;

typedef struct PsRegisters {
    // General-purpose registers.
    float floatVars[16];
} PsRegisters;


typedef struct ConstantPsScene {
    ConstantPsLight dirLight;
    ConstantPsLight pointLights[8];

    float fogMin, fogMax;
    int unused[2]; // Alignment.
    float fogColor[4];
} ConstantPsScene;

typedef struct ConstantPs {
    ConstantPsTex tex[8];

    PsRegisters regs;
} ConstantPs;





/*
   Global state.
*/
DRAWSTATE drawState;
//LIGHT dirLight;
//LIGHT PointLights[8];
//CAMERA3D camera3Handle;

uint32_t fogColor;
float fogMin, fogMax;

static mat4 identMat{1.0f};

/*
   Direct3D variables.
*/

static ID3D11Device* d3device = NULL;
static ID3D11DeviceContext* immediateContext = NULL;
static IDXGISwapChain* pSwapchain = NULL;

// Sampler.
static ID3D11SamplerState* linearSampler = NULL;
static ID3D11SamplerState* pointSampler = NULL;
// Raster.
static ID3D11RasterizerState* raster2D = NULL;
static ID3D11RasterizerState* raster3D = NULL;
static ID3D11RasterizerState* raster3DInvert = NULL;
// Depth stencil.
static ID3D11DepthStencilState* depthStencil3D = NULL;
static ID3D11DepthStencilState* depthStencil3DNoWrite = NULL;
static ID3D11DepthStencilState* depthStencil2D = NULL;
// Blending modes.
static ID3D11BlendState* blendStates[BLEND_COUNT];

// Vertex and index buffer.
static ID3D11Buffer* streamVertexBuffer = NULL;
static ID3D11Buffer* streamIndexBuffer = NULL;
static ID3D11Buffer* constantVSBuffer = NULL;
static ID3D11Buffer* constantPSBuffer = NULL;
static ID3D11Buffer* constantPSSceneBuffer = NULL;
static int currNverts;
static int currNindices;
static D3D11_MAPPED_SUBRESOURCE streamVertexMappedBuffer;
static D3D11_MAPPED_SUBRESOURCE streamIndexMappedBuffer;

//static ID3D11Texture2D* backBuffer = NULL;

// DXGI.
static IDXGIFactory* dxgiFactory = NULL;
static IDXGIAdapter* dxgiAdapter = NULL;
static IDXGIOutput* adapterOutput = NULL;
static char videoCardDescription[128];

static D3D11_VIEWPORT viewport;

typedef struct TargetSurface {
    GFX_texture color;
    GFX_texture depthStencil;
    ID3D11RenderTargetView* colorView;
    ID3D11DepthStencilView* depthStencilView;
} TargetSurface;

static TargetSurface surface1;
static TargetSurface surface2;
static ID3D11Texture2D* framebuffer = NULL;
static ID3D11RenderTargetView* framebufferView = NULL;

static float floatRegs[16];


/*
   Custom things.
*/
//static MESH utahTeapot;
typedef struct GFX_shader GFX_shader;
static GFX_shader* defaultShader = NULL;



//static FONT* defaultFont = NULL;
//static TEXTURE* fontTexture = NULL;
//static TEXTURE* defaultSquareTexture = NULL;



// Timing.
//#define DTIME 20

//static u64 startedTime = 0;
//static u64 deltaTimeU = 0;



typedef struct VertexShader {
    ID3D11VertexShader* vs;
    ID3D11InputLayout* inputLayout;

    VertexShader() : vs(NULL), inputLayout(NULL) {}
    VertexShader(ID3D11VertexShader* vs, ID3D11InputLayout* layout) : vs(vs), inputLayout(layout) {}
} VertexShader;

typedef struct GFX_shader {
    VertexShader vertexShader;
    ID3D11PixelShader* pixelShader;
} GFX_shader;


static std::map<const char*, VertexShader> shaderVsMap;
static std::map<const char*, ID3D11PixelShader*> shaderPsMap;




/*
   Static functions.
*/

static int HandleErrorShader(ID3D10Blob* errorBlob)
{
    if (errorBlob)
    {
        fprintf(stderr,
            "\nShader compilation failed! [error]:\n"
            "========================\n"
            "%s"
            "========================\n",
            (char *) (errorBlob->GetBufferPointer()));
        return 0;
    }

    return -1;
}


extern GFX_shader* ShaderLoad(ShaderLayout format, const char* vertexKey, const char* pixelKey)
{

    // HLSL file contains both the vertex shader and the fragment shader.
    // We use 2 DX compiler calls to compile one HLSL file into two VS and PS-formatted
    // shaders, which we'll then load.

    // Create shaders.
    GFX_shader* shd;

    VertexShader vertexShader;
    auto vs = shaderVsMap.find(vertexKey);
    if (vs != shaderVsMap.end())
    {
        vertexShader = vs->second;
    }
    else
    {
        Asset* asset = AssetLoad(vertexKey);
        if (asset == NULL)
        {
            fprintf(stderr, "Failed to load vertex shader! %s\n", vertexKey);
            return NULL;
        }

        HRESULT hr = d3device->CreateVertexShader(asset->data, asset->size, NULL, &vertexShader.vs);
        if (FAILED(hr))
        {
            fprintf(stderr, "Failed to create vertex shader! %s\n", vertexKey);
            AssetClose(asset);
            return NULL;
        }

        // Set input layout.
        if (format == SHADER_LAYOUT_SKINNED_ANIMATION)
        {
            D3D11_INPUT_ELEMENT_DESC layout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TANGENTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "JOINTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            d3device->CreateInputLayout(layout, 7, asset->data,
                asset->size, &vertexShader.inputLayout);
        }
        else if (format == SHADER_LAYOUT_FLATCOLOR)
        {
            D3D11_INPUT_ELEMENT_DESC layout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            d3device->CreateInputLayout(layout, 2, asset->data,
                asset->size, &vertexShader.inputLayout);

        }
        else if (format == SHADER_LAYOUT_BASIC_SHADED)
        {
            D3D11_INPUT_ELEMENT_DESC layout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            d3device->CreateInputLayout(layout, 4, asset->data,
                asset->size, &vertexShader.inputLayout);
        }
        else
        {
            fprintf(stderr, "%s: Invalid shader layout!\n", __func__);
            AssetClose(asset);
            return NULL;
        }

        AssetClose(asset);

        shaderVsMap.insert({ vertexKey, vertexShader });
    }

    ID3D11PixelShader* pixelShader;
    auto ps = shaderPsMap.find(pixelKey);
    if (ps != shaderPsMap.end())
    {
        pixelShader = ps->second;
    }
    else
    {
        Asset* asset = AssetLoad(pixelKey);
        if (asset == NULL)
        {
            fprintf(stderr, "Failed to load pixel shader! %s\n", pixelKey);
            return NULL;
        }
        HRESULT hr = d3device->CreatePixelShader(asset->data, asset->size, NULL, &pixelShader);
        if (FAILED(hr))
        {
            fprintf(stderr, "Failed to create pixel shader! %s\n", pixelKey);
            AssetClose(asset);
            return NULL;
        }

        AssetClose(asset);

        shaderPsMap.insert({ pixelKey, pixelShader });
    }

    shd = new GFX_shader;
    shd->vertexShader = vertexShader;
    shd->pixelShader = pixelShader;

    return shd;
}

extern void ShaderTerminate(GFX_shader* shader)
{
    delete shader;
}
extern int ShaderUse(GFX_shader* shader)
{
    if (shader)
    {
        immediateContext->IASetInputLayout(shader->vertexShader.inputLayout);

        immediateContext->VSSetShader(shader->vertexShader.vs, NULL, 0);
        immediateContext->PSSetShader(shader->pixelShader, NULL, 0);

        return 0;
    }
    return -1;
}

extern void ShaderSet(float value, int index)
{
    floatRegs[index] = value;
}


static void DoGamma(float* r, float* g, float* b)
{
    *r = 0.75f * (*r * *r) + 0.25f * *r * (*r * *r);
    *g = 0.75f * (*g * *g) + 0.25f * *g * (*g * *g);
    *b = 0.75f * (*b * *b) + 0.25f * *b * (*b * *b);
}

static void CopyLight(DrawPass* pass, ConstantPsLight* dest, GFX_light* l, b32 pointSource)
{
    if (pointSource)
    {
        vec4 v = mat4 { pass->viewMatrix->c } * vec4 { l->x, l->y, l->z };
        dest->position[0] = v.x;
        dest->position[1] = v.y;
        dest->position[2] = v.z;
    }
    else
    {
        mat4 m = mat4 { pass->viewMatrix->c };
        m.Inverse3();
        m = m.Transpose();

        vec4 v = m * vec4 { l->x, l->y, l->z };
        dest->position[0] = v.x;
        dest->position[1] = v.y;
        dest->position[2] = v.z;
    }

    dest->ambient[0] = l->ambR;
    dest->ambient[1] = l->ambG;
    dest->ambient[2] = l->ambB;

    dest->diffuse[0] = l->difR;
    dest->diffuse[1] = l->difG;
    dest->diffuse[2] = l->difB;

    dest->specular[0] = l->spcR;
    dest->specular[1] = l->spcG;
    dest->specular[2] = l->spcB;

    dest->constant = l->constant;
    dest->linear = l->linear;
    dest->quadratic = l->quadratic;
    dest->intensity = l->intensity;
}

static void SetNormalMat(void)
{
    drawState.normalMat = drawState.matStack[drawState.matStackIdx];
    drawState.normalMat.Inverse3();
    drawState.normalMat = drawState.normalMat.Transpose();
    drawState.normalMatValid = TRUE;
}

static void SetScenePSConstants(void)
{
    DrawPass* pass = &drawState.passes[drawState.currentPass];

    D3D11_MAPPED_SUBRESOURCE mappedBuffer;
    immediateContext->Map(constantPSSceneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);

    ConstantPsScene* scene = static_cast<ConstantPsScene *> (mappedBuffer.pData);
    // Copy the lights.
    CopyLight(pass, &scene->dirLight, pass->dirLight, FALSE);
    for (int i = 0; i < DRAW_MAX_POINTLIGHTS; ++i) {
        CopyLight(pass, &scene->pointLights[i], &pass->pointLights[i], TRUE);
    }
    // Fog.
    float fog[3] = {
        ((fogColor >> 16) & 0xFF) / 255.0f,
        ((fogColor >> 8) & 0xFF) / 255.0f,
        ((fogColor) & 0xFF) / 255.0f,
    };
    DoGamma(&fog[0], &fog[1], &fog[2]);

    scene->fogColor[0] = fog[0];
    scene->fogColor[1] = fog[1];
    scene->fogColor[2] = fog[2];
    scene->fogMin = fogMin;
    scene->fogMax = fogMax;

    immediateContext->Unmap(constantPSSceneBuffer, 0);
}

static void Draw_SetConstants(mat4* model)
{
    DrawPass* pass = &drawState.passes[drawState.currentPass];
    // VERTEX CONSTANT BUFFER.
    D3D11_MAPPED_SUBRESOURCE matRes;
    immediateContext->Map(constantVSBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &matRes);

    ConstantVs* mats = static_cast<ConstantVs *> (matRes.pData);

    // Fill modelview.
    if (model)
    {
        mats->modelView = *pass->viewMatrix * *model;
    }
    else
    {
        mats->modelView = *pass->viewMatrix;
    }


    // Fill normalMat.
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

    // Fill projection.
    DrawGetProjection(&mats->projection);

    // Etc.
    immediateContext->Unmap(constantVSBuffer, 0);


    // CONSTANT PIXEL SHADER BUFFER.
    D3D11_MAPPED_SUBRESOURCE mappedPS;
    immediateContext->Map(constantPSBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedPS);

    ConstantPs* ps = static_cast <ConstantPs *> (mappedPS.pData);
    for (int i = 0; i < DRAW_MAX_TEX; ++i)
    {
        ps->tex[i].offs[0] = drawState.tex[i].x;
        ps->tex[i].offs[1] = drawState.tex[i].y;
        ps->tex[i].scale[0] = drawState.tex[i].xs;
        ps->tex[i].scale[1] = drawState.tex[i].ys;
    }
    immediateContext->Unmap(constantPSBuffer, 0);

    // Set up registers.
    for (int i = 0; i < 16; ++i)
    {
        ps->regs.floatVars[i] = floatRegs[i];
    }

    // -------------------------------------------------------------
    // Set up texture handlers.
    ID3D11ShaderResourceView* textures[8];
    for (int i = 0; i < drawState.nTex; ++i)
    {
        if (drawState.tex[i].tex) {
            textures[i] = (ID3D11ShaderResourceView *) (drawState.tex[i].tex->resourceView);
        } else {
            textures[i] = NULL;
        }
    }

    immediateContext->PSSetShaderResources(0, drawState.nTex, textures);
    if (drawState.tex[0].tex && drawState.tex[0].tex->flags & TEXTURE_POINT)
    {
        immediateContext->PSSetSamplers(0, 1, &pointSampler);
    }
    else
    {
        immediateContext->PSSetSamplers(0, 1, &linearSampler);
    }


    // Then link the constant buffers. (two buffers)
    immediateContext->VSSetConstantBuffers(0, 1, &constantVSBuffer);
    if (pass->flags & DRAW_PASS_FLAG_SCENE_CONSTANTS)
    {
        ID3D11Buffer* buffers[2] = { constantVSBuffer, constantPSBuffer };
        immediateContext->PSSetConstantBuffers(0, 2, buffers);
    }
    else
    {
        immediateContext->PSSetConstantBuffers(0, 1, &constantPSBuffer);
        //fprintf(stderr, "%s: Ok bro\n", __func__);
    }
}



static void DrawPrepare(void)
{
    if (!drawState.hasBuffer)
    {
        // Accumulate vertices.
        immediateContext->Map(streamVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &streamVertexMappedBuffer);
        immediateContext->Map(streamIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &streamIndexMappedBuffer);

        currNindices = 0;
        currNverts = 0;

        drawState.hasBuffer = TRUE;
    }
}
extern void DrawFlush(void)
{
    if (drawState.hasBuffer)
    {
        immediateContext->Unmap(streamVertexBuffer, 0);
        streamVertexMappedBuffer.pData = NULL;
        immediateContext->Unmap(streamIndexBuffer, 0);
        streamIndexMappedBuffer.pData = NULL;

        Draw_SetConstants(NULL);

        UINT stride = sizeof(CustomVertexDX11);
        UINT offset = 0;

        // TODO: bruhh this has to be the issue.
        immediateContext->IASetVertexBuffers(0, 1, &streamVertexBuffer, &stride, &offset);
        immediateContext->IASetIndexBuffer(streamIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        immediateContext->DrawIndexed(currNindices, 0, 0);

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

    int maxVerts = VBO_MAX_SIZE / sizeof(CustomVertexDX11);
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

    CustomVertexDX11* d = &((CustomVertexDX11 *) streamVertexMappedBuffer.pData)[currNverts];

    // TODO: It needs to be aligned.
    d->x = pos.x;
    d->y = pos.y;
    d->z = pos.z;
    d->nx = normal.x;
    d->ny = normal.y;
    d->nz = normal.z;

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
    d->r = r;
    d->g = g;
    d->b = b;
    d->a = a;

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
        ((int *) (streamIndexMappedBuffer.pData))[currNindices++] = base + st[i];
    }
}

extern void DrawPreFlush(int nverts, int nindices)
{
    int maxVerts = VERTEX_BUFFER_SIZE / sizeof(CustomVertexDX11);
    int maxIndices = INDEX_BUFFER_SIZE / sizeof(int);
    if (currNverts + nverts >= maxVerts ||
        currNindices + nindices >= maxIndices) {
        DrawFlush();
    }
}

extern void DrawBegin(void)
{
}

extern void DrawEnd(void)
{
    // Lock to screen refresh rate with 1.
    pSwapchain->Present(1, 0);
}


// Declared by draw3d.h.
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
    }
}

extern GFX_texture* DrawGetFboTexture(int what)
{
    switch (what)
    {
        case FBO_TEXTURE1_COLOR:
            return &surface1.color;
        case FBO_TEXTURE1_DEPTH:
            return &surface1.depthStencil;
        case FBO_TEXTURE2_COLOR:
            return &surface2.color;
        case FBO_TEXTURE2_DEPTH:
            return &surface2.depthStencil;

        default:
            return NULL;
    }
}



extern void DrawBlend(BlendMode blend)
{
    if (drawState.blend != blend)
    {
        // The issue is at draw flush.
        DrawFlush();
        drawState.blend = blend;

        immediateContext->OMSetBlendState(blendStates[blend], NULL, 0xffffffff);
    }
}


extern void DrawZBufferWrite(b32 flag)
{
    if (drawState.zWrite != flag && // Crashes at passes.
        drawState.passes[drawState.currentPass].depthStencilMode != DEPTH_STENCIL_DISABLE)
    {
        DrawFlush();

        if (flag) {
            immediateContext->OMSetDepthStencilState(depthStencil3D, 0);
        } else {
            immediateContext->OMSetDepthStencilState(depthStencil3DNoWrite, 0);
        }
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

extern void DrawWireframe(b32 enable)
{
    if (drawState.wireframe != enable)
    {
        DrawFlush();
        drawState.wireframe = enable;
    }
}

extern void DrawCullInvert(b32 invert)
{
    if (drawState.cullInvert != invert && drawState.passes[drawState.currentPass].cullMode != CULL_NONE)
    {
        DrawFlush();
        if (invert) {
            immediateContext->RSSetState(raster3DInvert);
        } else {
            immediateContext->RSSetState(raster3D);
        }
    }
    drawState.cullInvert = invert;
}

extern void DrawClear(uint32_t color)
{
    float ccl[4] = {
        ((color >> 16) & 0xff) / 255.0f,
        ((color >> 8 ) & 0xff) / 255.0f,
        ((color      ) & 0xff) / 255.0f,
        1.0f,
    };
    if (drawState.passes[drawState.currentPass].flags & DRAW_PASS_FLAG_GAMMA)
    {
        DoGamma(&ccl[0], &ccl[1], &ccl[2]);
    }


    TargetSurface* target = NULL;
    if (drawState.passes[drawState.currentPass].target == 0)
    {
        target = &surface1;
    }
    else if (drawState.passes[drawState.currentPass].target == 1)
    {
        target = &surface2;
    }


    if (target)
    {
        immediateContext->ClearRenderTargetView(target->colorView, ccl);
        immediateContext->ClearDepthStencilView(target->depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }
    else
    {
        immediateContext->ClearRenderTargetView(framebufferView, ccl);
    }
}

extern void DrawSetTarget(void)
{
    DrawPass* pass = &drawState.passes[drawState.currentPass];

    ID3D11ShaderResourceView* nullView = NULL;
    immediateContext->PSSetShaderResources(0, 1, &nullView);

    if (pass->target == 0)
    {
        immediateContext->OMSetRenderTargets(1, &surface1.colorView, surface1.depthStencilView);
    }
    else if (pass->target == 1)
    {
        immediateContext->OMSetRenderTargets(1, &surface2.colorView, surface2.depthStencilView);
    }
    else
    {
        immediateContext->OMSetRenderTargets(1, &framebufferView, NULL);
    }


    // Set depth stencil mode.
    switch (pass->depthStencilMode)
    {
        case DEPTH_STENCIL_DISABLE:
            immediateContext->OMSetDepthStencilState(depthStencil2D, 0);
            break;
        case DEPTH_STENCIL_DEPTH:
            immediateContext->OMSetDepthStencilState(depthStencil3D, 0);
            break;
        case DEPTH_STENCIL_NO_WRITE:
            immediateContext->OMSetDepthStencilState(depthStencil3DNoWrite, 0);
            break;
    }


    // Create the viewport.
    D3D11_VIEWPORT viewport = { pass->viewportX, pass->viewportY, pass->viewportW, pass->viewportH, 0, 1 };
    immediateContext->RSSetViewports(1, &viewport);

    // And then culling mode.
    switch (pass->cullMode)
    {
        case CULL_NONE:
            immediateContext->RSSetState(raster2D);
            break;
        case CULL_BACK:
            immediateContext->RSSetState(raster3D);
            break;
        case CULL_FRONT:
            immediateContext->RSSetState(raster3DInvert);
            break;
    }
}

// Uploads immutable data.
extern GFX_mesh* DrawUploadMesh(void* alloc, void* verts, void* indices, int nVertices)
{
    puts("TODO: Implement DrawUploadMesh");
    /*
    size_t vertexSize = sizeof(CustomVertexDX11);

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = vertexSize * m->nvertices;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA subres;
    subres.pSysMem = verts;
    subres.SysMemPitch = vertexSize;
    subres.SysMemSlicePitch = 0;

    ID3D11Buffer* buffer;
    if (FAILED(d3device->CreateBuffer(&bd, NULL, &buffer)))
    {
        fprintf(stderr, "%s: Failed to create buffer!\n", __func__);
        return -1;
    }
    m->vertexBuffer = buffer;

    bd.ByteWidth = m->nfaces * 12;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    subres.pSysMem = indices;
    subres.SysMemPitch = 4; // 4 bytes per integer.
    if (FAILED(d3device->CreateBuffer(&bd, NULL, &buffer)))
    {
        fprintf(stderr, "%s: Failed to create buffer!\n", __func__);
        return -1;
    }
    m->indexBuffer = buffer;
    */

    return 0;
}

extern void DrawMeshTerminate(GFX_mesh* m)
{
    /*
    ID3D11Buffer* buf;
    buf = (ID3D11Buffer *) m->vertexBuffer;
    buf->Release();
    buf = (ID3D11Buffer *) m->indexBuffer;
    buf->Release();

    MeshTerminate(m);
    */
}

extern void DrawRenderMesh(GFX_mesh* m)
{
    /*
    DrawFlush();
    Draw_SetConstants(&drawState.matStack[drawState.matStackIdx]);

    ID3D11Buffer* verts = (ID3D11Buffer *) m->vertices;
    ID3D11Buffer* indices = (ID3D11Buffer *) m->indices;

    UINT stride = sizeof(vertex_t);
    UINT offset = 0;

    immediateContext->IASetVertexBuffers(0, 1, &verts, &stride, &offset);
    immediateContext->IASetIndexBuffer(indices, DXGI_FORMAT_R32_UINT, 0);
    immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    immediateContext->DrawIndexed(m->nfaces * 3, 0, 0);
    */
}


static int CreateTargetSurface(TargetSurface* surface)
{
    // We use the Window Handle.
    D3D11_TEXTURE2D_DESC texDesc = { 0 };
    texDesc.Width = windowHandle->w;
    texDesc.Height = windowHandle->h;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* texture;
    HRESULT hr = d3device->CreateTexture2D(&texDesc, NULL, &texture);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create texture!\n", __func__);
        return -1;
    }


    surface->color.handle = texture;
    hr = d3device->CreateRenderTargetView(texture, NULL, &surface->colorView);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create render target view!\n", __func__);
        return -1;
    }


    // Shader view.
    ID3D11ShaderResourceView* shaderView;
    hr = d3device->CreateShaderResourceView(texture, NULL, &shaderView);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create shader resource view!\n", __func__);
        return -1;
    }
    surface->color.resourceView = shaderView;
    surface->color.w = windowHandle->w;
    surface->color.h = windowHandle->h;
    surface->color.refs = 1;


    // Reuse for depth stencil.
    texDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    hr = d3device->CreateTexture2D(&texDesc, NULL, &texture);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create texture!\n", __func__);
        return -1;
    }
    surface->depthStencil.handle = texture;

    hr = d3device->CreateDepthStencilView(texture, NULL, &surface->depthStencilView);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create depth stencil view!\n", __func__);
        return -1;
    }
    surface->depthStencil.w = windowHandle->w;
    surface->depthStencil.h = windowHandle->h;
    surface->depthStencil.flags |= TEXTURE_POINT;
    surface->depthStencil.refs = 1;


    return 0;
}

extern void ReleaseTexture(GFX_texture* texture);

static void DeleteTargetSurface(TargetSurface* surface)
{
    surface->depthStencilView->Release();
    ReleaseTexture(&surface->depthStencil);
    surface->colorView->Release();
    ReleaseTexture(&surface->color);
}

static HRESULT InitDirect3D(Window* win)
{
    fprintf(stderr, "%s: Initializing DirectX...\n", __func__);

    HRESULT hr = S_OK;

    //RECT rc;
    //GetClientRect(win->backend->hwnd, &rc);
    //UINT width = rc.right - rc.left;
    //UINT height = rc.top - rc.bottom;


    hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **) &dxgiFactory);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create DXGI factory!\n", __func__);
        return -1;
    }
    // Use the factory to create an adapter
    // for the primary graphics interface (video card).
    hr = dxgiFactory->EnumAdapters(0, &dxgiAdapter);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to enumerate adapters!\n", __func__);
        return -1;
    }
    // Enumerate the primary adapter output (monitor).
    hr = dxgiAdapter->EnumOutputs(0, &adapterOutput);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to enumerate monitor outputs!\n", __func__);
        return -1;
    }


    // Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format
    // for the adapter output (monitor).
    unsigned int numModes = 0;
    hr = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to get display mode list!\n", __func__);
        return -1;
    }
    // Create a list to hold all possible display modes.
    DXGI_MODE_DESC* displayModeList = (DXGI_MODE_DESC *) _MALLOC(sizeof(DXGI_MODE_DESC) * numModes);
    if (!displayModeList)
    {
        fprintf(stderr, "%s: Failed to allocate display mode list! OOM?\n", __func__);
        return -1;
    }
    // Call the same function 2 times, sort of like in Vulkan.
    hr = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Faield to get display mode list!\n", __func__);
        return -1;
    }

    // Now go through the display modes.
    int numerator = -1000, denominator = 1000;
    for (int i = 0; i < numModes; ++i)
    {
        if (displayModeList[i].Width == (unsigned int) win->w)
        {
            if (displayModeList[i].Height == (unsigned int) win->h)
            {
                numerator = displayModeList[i].RefreshRate.Numerator;
                denominator = displayModeList[i].RefreshRate.Denominator;
            }
        }
    }

    fprintf(stderr, "%s: YOO win w:%d, win h:%d\n", __func__, win->w, win->h);
    fprintf(stderr, "%s: hwnd: %p\n", __func__, win->backend->hwnd);

    // Get the adapter (video card) description.
    DXGI_ADAPTER_DESC adapterDesc;
    hr = dxgiAdapter->GetDesc(&adapterDesc);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to get adapter desc (properties)!\n", __func__);
        return -1;
    }


    int videoCardMemory;
    videoCardMemory = (int) (adapterDesc.DedicatedVideoMemory / 1024 / 1024);

    // Strcpy basically.
    int err;
    unsigned long long stringLength;
    err = wcstombs_s(&stringLength, videoCardDescription, 128, adapterDesc.Description, 128);
    if (err != 0) {
        return -1;
    }

    fprintf(stderr, "%s: video card: %s\n", __func__, videoCardDescription);

    // Release.
    _FREE(displayModeList);
    displayModeList = 0;

    adapterOutput->Release();
    adapterOutput = 0;

    dxgiAdapter->Release();
    dxgiAdapter = 0;

    dxgiFactory->Release();
    dxgiFactory = 0;


    // Then the swapchain.
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    // Descriptor.
    sd.BufferDesc.Width = win->w;
    sd.BufferDesc.Height = win->h;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //if (VSYNC_ENABLED) {
        sd.BufferDesc.RefreshRate.Numerator = numerator;
        sd.BufferDesc.RefreshRate.Denominator = denominator;
    //} else {
    //   sd.BufferDesc.RefreshRate.Numerator = 60; // 60 FPS.
    //    sd.BufferDesc.RefreshRate.Denominator = 1;
    //}
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = win->backend->hwnd;
    // Turn multisampling off.
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    fprintf(stderr, "hwnd: %p\n", win->backend->hwnd);

#define FULLSCREEN 0
    if (FULLSCREEN) {
        sd.Windowed = FALSE;
    } else {
        sd.Windowed = TRUE;
    }
    // Set the scanline ordering and scaling to be unspecified.
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    // Discard the backbuffer contents after presenting.
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
//#ifndef RELEASE 
    sd.Flags |= D3D11_CREATE_DEVICE_DEBUG;
//#endif 


    // We get our features and device with swapchain.
    // It creates the device, device (immediate) context, and swapchain.
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE,
            NULL, 0, &featureLevel, 1, D3D11_SDK_VERSION, &sd,
            &pSwapchain, &d3device, NULL, &immediateContext);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create device and swapchain!\n", __func__);
        return hr;
    }


    // Create render target view.
    hr = pSwapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *) &framebuffer);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Couldn't get back buffer!\n", __func__);
        return hr;
    }
    hr = d3device->CreateRenderTargetView(framebuffer, NULL, &framebufferView);
    if (FAILED(hr))
    {
        return hr;
    }



    // We reuse our descriptors.
    D3D11_BUFFER_DESC bufferDesc = { 0 };
    bufferDesc.ByteWidth = VERTEX_BUFFER_SIZE;
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = d3device->CreateBuffer(&bufferDesc, NULL, &streamVertexBuffer);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create master vertex buffer!\n", __func__);
        return -1;
    }

    bufferDesc.ByteWidth = INDEX_BUFFER_SIZE;
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = d3device->CreateBuffer(&bufferDesc, NULL, &streamIndexBuffer);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create master index buffer!\n", __func__);
        return -1;
    }

    bufferDesc.ByteWidth = sizeof(ConstantVs);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = d3device->CreateBuffer(&bufferDesc, NULL, &constantVSBuffer);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create constant vertex shader buffer!\n", __func__);
        return -1;
    }

    bufferDesc.ByteWidth = sizeof(ConstantPs);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = d3device->CreateBuffer(&bufferDesc, NULL, &constantPSBuffer);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create constant pixel shader buffer!\n", __func__);
        return -1;
    }

    bufferDesc.ByteWidth = sizeof(ConstantPsScene);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = d3device->CreateBuffer(&bufferDesc, NULL, &constantPSSceneBuffer);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create scene buffer!\n", __func__);
        return -1;
    }

    // Rasterizers.
    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZeroMemory(&rasterizerDesc, sizeof(rasterizerDesc));
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.FrontCounterClockwise = TRUE;
    rasterizerDesc.DepthClipEnable = TRUE;
    d3device->CreateRasterizerState(&rasterizerDesc, &raster2D);
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    d3device->CreateRasterizerState(&rasterizerDesc, &raster3D);
    rasterizerDesc.CullMode = D3D11_CULL_FRONT;
    d3device->CreateRasterizerState(&rasterizerDesc, &raster3DInvert);


    // Samplers.
    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(samplerDesc));
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    hr = d3device->CreateSamplerState(&samplerDesc, &linearSampler);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create linear sampler!\n", __func__);
        return -1;
    }
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    hr = d3device->CreateSamplerState(&samplerDesc, &pointSampler);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create point sampler!\n", __func__);
        return -1;
    }

    // Blending.
    D3D11_BLEND_DESC blendDesc;
    ZeroMemory(&blendDesc, sizeof(blendDesc));
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    hr = d3device->CreateBlendState(&blendDesc, &blendStates[BLEND_ALPHA]);
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    hr = d3device->CreateBlendState(&blendDesc, &blendStates[BLEND_MULTIPLY]);
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    hr = d3device->CreateBlendState(&blendDesc, &blendStates[BLEND_ADD]);
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    hr = d3device->CreateBlendState(&blendDesc, &blendStates[BLEND_REPLACE]);
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_COLOR;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_COLOR;
    hr = d3device->CreateBlendState(&blendDesc, &blendStates[BLEND_SCREEN]);
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    hr = d3device->CreateBlendState(&blendDesc, &blendStates[BLEND_SUBTRACT]);


    drawState.blend = static_cast <BlendMode> (-1);
    _MEMSET(floatRegs, 0, sizeof(floatRegs));


    // Depth stencil.
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = { 0 };
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    depthStencilDesc.StencilEnable = FALSE;
    hr = d3device->CreateDepthStencilState(&depthStencilDesc, &depthStencil3D);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create depth stencil desc 3D!\n", __func__);
        return -1;
    }
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    hr = d3device->CreateDepthStencilState(&depthStencilDesc, &depthStencil3DNoWrite);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create depth stencil desc 3D no write!\n", __func__);
        return -1;
    }
    depthStencilDesc.DepthEnable = FALSE;
    hr = d3device->CreateDepthStencilState(&depthStencilDesc, &depthStencil2D);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create depth stencil desc 2D!\n", __func__);
        return -1;
    }

    CreateTargetSurface(&surface1);
    CreateTargetSurface(&surface2);

    surface2.color.flags |= TEXTURE_POINT;

    return S_OK;
}


/*
   Public interface.
*/

// Do we serve any more spaghetti here?
static DrawPass _passes[1] = { 0 };
static mat4 _viewMat = mat4{1.0f};

extern int GraphicsInit(Window* win)
{
    drawState.drawArena = ArenaInit(MB(2), KB(64), ARENA_FLAG_GROWABLE);

    // Init graphics.
    if (FAILED(InitDirect3D(win)))
    {
        fprintf(stderr, "%s: Failed to initialize Direct3D!\n", __func__);
        return -1;
    }

    // UTAH EXAMPLE.
    DrawSetPipeline(1, _passes);
    DrawSetPass2D(&_passes[0]);
    _passes[0].active = TRUE;
    _passes[0].viewMatrix = &_viewMat;
    _passes[0].flags |= DRAW_PASS_FLAG_GAMMA;
    DrawSetTarget();

    // Init shader and buffer.
    defaultShader = ShaderLoad(SHADER_LAYOUT_BASIC_SHADED, "assets/shaders/base.vso", "assets/shaders/base.pso");
    if (defaultShader == NULL)
    {
        fprintf(stderr, "%s: Failed to load shader!\n", __func__);
        return -1;
    }

     return 0;
}

extern void GraphicsTerminate(void)
{
    // Before shutting down set to windowed mode,
    // otherwise when you release the swapchain it will throw an exception.
    if (pSwapchain)
    {
        pSwapchain->SetFullscreenState(FALSE, NULL);
    }

    DeleteTargetSurface(&surface1);
    DeleteTargetSurface(&surface2);

    depthStencil3D->Release();
    depthStencil3DNoWrite->Release();
    depthStencil2D->Release();

    for (int i = 0; i < BLEND_COUNT; ++i)
    {
        if (blendStates[i]) {
            blendStates[i]->Release();
        }
    }

    linearSampler->Release();
    pointSampler->Release();
    raster2D->Release();
    raster3D->Release();
    raster3DInvert->Release();

    // USER STUFF.

    // Remove basic shader.
    //defaultShader->vertexShader.vs->Release();
    //defaultShader->vertexShader.inputLayout->Release();
    //defaultShader->pixelShader->Release();
    ShaderTerminate(defaultShader);

    //TextureTerminate(defaultSquareTexture);
    ////////////////////////////// 



    streamVertexBuffer->Release();
    streamIndexBuffer->Release();
    constantVSBuffer->Release();
    constantPSBuffer->Release();
    constantPSSceneBuffer->Release();

    // WARN: Framebuffer is probably the issue.
    framebufferView->Release();
    framebuffer->Release();

    // Then the swapchain is released.
    pSwapchain->Release();


#ifndef RELEASE 
    ID3D11Debug* debug;
    HRESULT hr = d3device->QueryInterface<ID3D11Debug>(&debug);
    if (SUCCEEDED(hr))
    {
        debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
        debug->Release();
    }
    fprintf(stderr, "%s: Ended debug!\n", __func__);
#endif

    d3device->Release();
    immediateContext->Release();

    /*
    // Shut down the graphics.
    if (rasterState) {
        rasterState->Release();
        rasterState = 0;
    }
    if (depthStencilView) {
        depthStencilView->Release();
        depthStencilView = 0;
    }
    if (depthStencilState) {
        depthStencilState->Release();
        depthStencilState = 0;
    }
    if (depthStencilBuffer) {
        depthStencilBuffer->Release();
        depthStencilBuffer = 0;
    }
    if (renderTargetView) {
        renderTargetView->Release();
        renderTargetView = 0;
    }
    if (immediateContext) {
        immediateContext->Release();
        immediateContext = 0;
    }
    if (d3device) {
        d3device->Release();
        d3device = 0;
    }
    if (pSwapchain) {
        pSwapchain->Release();
        pSwapchain = 0;
    }

    // Shut down the window.
    //WindowTerminate(&windowHandle);
    */

    // Shut down the model.
    //MeshTerminate(&utahTeapot);
    //ShaderTerminate(defaultShader);
}


/*
   Texture functions.
*/

extern GFX_texture* LoadImmutableTextureFromPixels(void* alloc, int w, int h, unsigned char* pix, int flags)
{
    D3D11_TEXTURE2D_DESC textureDesc;
    ZeroMemory(&textureDesc, sizeof(textureDesc));
    textureDesc.Width = w;
    textureDesc.Height = h;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = (flags & TEXTURE_SRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    //textureDesc.SampleDesc.Quality = 1;
    textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.MiscFlags = 0;
    if (flags & TEXTURE_MIPMAP)
    {
        textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.MipLevels = 1;
    }

    D3D11_SUBRESOURCE_DATA textureData;
    textureData.pSysMem = pix;
    textureData.SysMemPitch = w * 4;

    ID3D11Texture2D* texture2D;
    HRESULT hr = d3device->CreateTexture2D(&textureDesc, &textureData, &texture2D);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to load immutable texture!\n", __func__);
        return NULL;
    }


    ID3D11ShaderResourceView* resourceView;
    hr = d3device->CreateShaderResourceView(texture2D, NULL, &resourceView);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to load shader resource view!\n", __func__);
        return NULL;
    }


    if (flags & TEXTURE_MIPMAP)
    {
        immediateContext->GenerateMips(resourceView);
    }

    GFX_texture* tex = (GFX_texture *) _MALLOC(sizeof(GFX_texture));
    tex->w = w;
    tex->h = h;
    tex->flags = flags;
    tex->refs = 1;
    tex->handle = texture2D;
    tex->resourceView = resourceView;

    return tex;
}

extern void ReleaseTexture(GFX_texture *texture)
{
    ID3D11ShaderResourceView* resView = (ID3D11ShaderResourceView *) texture->resourceView;
    if (resView)
    {
        resView->Release();
    }

    ID3D11Texture2D* texture2D = (ID3D11Texture2D *) texture->handle;
    texture2D->Release();

    texture->resourceView = NULL;
    texture->handle = NULL;
}

extern void TextureTerminate(void* alloc, GFX_texture *texture)
{
    if (!texture)
    {
        return;
    }

    texture->refs -= 1;
    if (texture->refs > 0)
    {
        return;
    }

    ReleaseTexture(texture);
    _FREE(texture);
}


extern GFX_texture* AllocateMutableTexture(int width, int height, void* args, int flags)
{
    D3D11_TEXTURE2D_DESC textureDesc = { 0 };
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = (flags & TEXTURE_SRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DYNAMIC;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    textureDesc.MiscFlags = 0;

    // No mipmap generation for dynamic textures, sorry!


    //D3D11_SUBRESOURCE_DATA textureData = { 0 };
    //textureData.pSysMem = NULL;
    //textureData.SysMemPitch = width * 4;

    ID3D11Texture2D* texture2D;
    HRESULT hr = d3device->CreateTexture2D(&textureDesc, NULL, &texture2D);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to load dynamic texture!\n", __func__);
        return NULL;
    }


    ID3D11ShaderResourceView* resourceView;
    hr = d3device->CreateShaderResourceView(texture2D, NULL, &resourceView);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to load shader resource view!\n", __func__);
        return NULL;
    }


    GFX_texture* tex = (GFX_texture *) _MALLOC(sizeof(GFX_texture));
    tex->w = width;
    tex->h = height;
    tex->flags = flags;
    tex->refs = 1;
    tex->handle = texture2D;
    tex->resourceView = resourceView;

    return tex;

}

extern unsigned char* TextureLock(
    GFX_texture* texture, unsigned int* rowPitch, unsigned int* depthPitch)
{
    ID3D11Texture2D* tex = static_cast<ID3D11Texture2D *> (texture->handle);

    D3D11_MAPPED_SUBRESOURCE mappedBuffer;
    immediateContext->Map(tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);

    unsigned char* result = static_cast<unsigned char *> (mappedBuffer.pData);
    if (rowPitch) *rowPitch = mappedBuffer.RowPitch;
    if (depthPitch) *depthPitch = mappedBuffer.DepthPitch;

    return result;
}

extern void TextureUnlock(GFX_texture* texture)
{
    ID3D11Texture2D* tex = static_cast<ID3D11Texture2D *> (texture->handle);

    immediateContext->Unmap(tex, 0);
}

/*
   3D textures.
*/

extern GFX_texture3D* AllocateMutableTexture3D(int width, int height, int depth, void* args, int flags)
{
    D3D11_TEXTURE3D_DESC textureDesc = { 0 };
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.Depth = depth;

    textureDesc.MipLevels = 1;
    textureDesc.Format = (flags & TEXTURE_SRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Usage = D3D11_USAGE_DYNAMIC;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    textureDesc.MiscFlags = 0;

    ID3D11Texture3D* texture3D;
    HRESULT hr = d3device->CreateTexture3D(&textureDesc, NULL, &texture3D);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to load dynamic 3D texture!\n", __func__);
        return NULL;
    }


    ID3D11ShaderResourceView* resourceView;
    hr = d3device->CreateShaderResourceView(texture3D, NULL, &resourceView);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to load shader resource view!\n", __func__);
        return NULL;
    }


    GFX_texture3D* tex = (GFX_texture3D *) _MALLOC(sizeof(GFX_texture3D));
    tex->w = width;
    tex->h = height;
    tex->flags = flags;
    tex->refs = 1;
    tex->handle = texture3D;
    tex->resourceView = resourceView;

    return tex;
}

