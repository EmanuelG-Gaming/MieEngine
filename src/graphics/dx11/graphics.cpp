#include "../draw.h"
#include "../../base/base_log.h"
#include "../../win/win.h"

#include <stddef.h>
#include <stdio.h>

#include <d3d11.h>

/*
   TODO: Use shader bytecode directly?
*/
#include <d3dcompiler.h>


// Multiline string.
#define D3D_HLSL_DECL(...) #__VA_ARGS__

static const char* color_vs = D3D_HLSL_DECL(
    cbuffer vs_constants : register(b0) {
        column_major float4x4 modelView; // Model matrix.
        column_major float4x4 projection;
        column_major float4x4 normMat;
    };

    // Typedefs.
    struct VertexInputType {
        float3 position: POSITION;
        //float3 color: COLOR;

        //float3 position: POSITION;
        
        //float4 tangents: TANGENTS;
        //float4 joints: JOINTS;
        //float4 weights: WEIGHTS;
        float3 normal : NORMAL;
        float2 texCoords : TEXCOORDS;

        float4 color: COLOR;
    };

    struct PixelInputType {
        float4 projPosition: SV_POSITION; // REQUIRED.

        float3 viewPos : POSITION;
        //float4 tangents: TANGENTS;
        //float4 joints: JOINTS;
        //float4 weights: WEIGHTS;

        float3 normal : NORMAL;
        float2 texCoords : TEXCOORDS;
        float4 color: COLOR;

    };

    // Vertex shader.
    PixelInputType VS(VertexInputType input) {
        PixelInputType output;

        float4 pos = mul(modelView, float4(input.position, 1.0f));
        //output.position = mul(viewMatrix, output.position);
        //output.position = mul(projectionMatrix, output.position);

        output.projPosition = mul(projection, pos);
        //output.projPosition = float4(input.position, 1.0f);
        output.viewPos = pos.xyz;

        output.normal = mul(normMat, float4(input.normal, 0.0)).xyz;
        output.texCoords = input.texCoords;
        output.color = input.color;

        return output;
    }
);

static const char* color_ps = D3D_HLSL_DECL(
    // Typedefs.
    /*
    struct PixelInputType {
    float4 position: SV_POSITION;
    float4 color: COLOR;
    };
    */

    struct PS_Light {
        float3 pos;
        float3 ambient;
        float3 diffuse;
        float3 specular;
        float constant;
        float lin;
        float quadratic;
        float intensity;
    };
    struct PS_Texture {
        float2 pos;
        float2 scale;
    };
    //#define N_POINTLIGHTS 8
    //#define N_TEXS 8

    cbuffer ps_constants : register(b0) {
        //PS_Texture tex[N_TEXS];
        PS_Texture tex[8];
    };
    cbuffer ps_light_constants : register(b1) {
        PS_Light dir_light;
        //PS_Light point_lights[N_POINTLIGHTS];
        PS_Light point_lights[8];

        float fog_min;
        float fog_max;
        float unused1;
        float unused2;
        float3 fog_color;
    };
    SamplerState my_sampler : register(s0);
    texture2D base_texture : register(t0);
    texture2D texture1 : register(t1);


    // Pixel shader.
    float4 PS(PixelInputType input) : SV_TARGET {
        return base_texture.Sample(my_sampler, input.texCoords) * input.color;
        //float z = (input.position.y * 0.095) +  (input.position.x  *  0.095);

        //return float4(input.position.y / 255, input.position.x / 255, z / 255, 1.0);
        //return input.color;
        //return float4(1, 1, 0, 1);
    }
);

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
   DirectX constant buffers.
*/

typedef struct ConstantVS {
    mat4 modelView;
    mat4 projection;
    mat4 normMat;
} ConstantVS;

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

typedef struct ConstantPsScene {
    //ConstantPsLight dirLight;
    //ConstantPsLight pointLights[8];

    float fogMin, fogMax;
    int unused[2]; // Alignment.
    float fogColor[4];
} ConstantPsScene;

typedef struct ConstantPS {
    ConstantPsTex tex[8];
} ConstantPS;





/*
   Render state.
*/

static int currNverts;
static int currNindices;

DRAWSTATE drawState;

static mat4 identMat = mat4 { 1.0f };

/*
   Direct3D variables.
*/

static ID3D11Device* d3device = NULL;
static ID3D11DeviceContext* immediateContext = NULL;
static IDXGISwapChain* pSwapchain = NULL;

// Sampler states.
static ID3D11SamplerState* linearSampler = NULL;
static ID3D11SamplerState* pointSampler = NULL;
// Raster states.
static ID3D11RasterizerState* raster2D = NULL;
static ID3D11RasterizerState* raster3D = NULL;
static ID3D11RasterizerState* raster3Dinvert = NULL;
// Depth stencils.
static ID3D11DepthStencilState* depthStencil3D = NULL;
static ID3D11DepthStencilState* depthStencil3DnoWrite = NULL;
static ID3D11DepthStencilState* depthStencil2D = NULL;
// Blending modes.
static ID3D11BlendState* blendStates[BLEND_COUNT];



// Vertex and index buffer.
static ID3D11Buffer* streamVertexBuffer = NULL;
static ID3D11Buffer* streamIndexBuffer = NULL;

static D3D11_MAPPED_SUBRESOURCE streamVertexMappedBuffer;
static D3D11_MAPPED_SUBRESOURCE streamIndexMappedBuffer;

// Constant buffers.
static ID3D11Buffer* constantVSBuffer = NULL;
static ID3D11Buffer* constantPSBuffer = NULL;
static ID3D11Buffer* constantPSSceneBuffer = NULL;


/*
   DXGI (are these really necessary)?
*/
static IDXGIFactory* dxgiFactory = NULL;
static IDXGIAdapter* dxgiAdapter = NULL;
static IDXGIOutput* adapterOutput = NULL;
static char videoCardDescription[128];



typedef struct TargetSurface {
    GFX_texture color;
    GFX_texture depthStencil;

    ID3D11RenderTargetView* colorView;
    ID3D11DepthStencilView* depthStencilView;
} TargetSurface;
static TargetSurface surface1;
static TargetSurface surface2;


// The 3rd target surface (the one needed to be shown to the screen).
static ID3D11Texture2D* framebuffer = NULL;
static ID3D11RenderTargetView* framebufferView = NULL;
static D3D11_VIEWPORT viewport;


/*
   Shader pipeline loader.
*/
typedef enum ShaderLayout {
    SHADER_LAYOUT_NONE = 0,
    SHADER_LAYOUT_FLATCOLOR,
    SHADER_LAYOUT_BASIC_SHADED,
    SHADER_LAYOUT_SKINNED_ANIMATION,
} ShaderLayout;

typedef struct VertexShader {
    ID3D11VertexShader* vs;
    ID3D11InputLayout* inputLayout;

    VertexShader() : vs(NULL), inputLayout(NULL) {}
    VertexShader(ID3D11VertexShader* vs, ID3D11InputLayout* layout) : vs(vs), inputLayout(layout) {}
} VertexShader;

typedef struct Shader {
    VertexShader vertexShader;
    ID3D11PixelShader* pixelShader;
} Shader;

static Shader* defaultShader = NULL;


static int HandleErrorShader(ID3D10Blob* errorBlob)
{
    if (errorBlob)
    {
        fprintf(stderr,
                "Shader compilation failed!\n"
                "==========================\n"
                "%s"
                "==========================\n",
                reinterpret_cast<char *> (errorBlob->GetBufferPointer()));
        return 0;
    }

    return -1;
}

static Shader* ShaderLoad(ShaderLayout format, const char* vs, const char* fs)
{
    // source code into one large string.
    char buf[4096];
    snprintf(buf, 2048, "%s %s", vs, fs);

    HRESULT hr = S_OK;

    ID3D10Blob* vertexShaderBlob = NULL;
    ID3D10Blob* pixelShaderBlob = NULL;
    ID3D10Blob* errorBlob = NULL;

    // Vertex shader.
    // Blobs.
    hr = D3DCompile(buf, strlen(buf), NULL, NULL, NULL, "VS", "vs_5_0", 0, 0, &vertexShaderBlob, &errorBlob);
    if (FAILED(hr))
    {
        HandleErrorShader(errorBlob);
        return NULL;
    }

    hr = D3DCompile(buf, strlen(buf), NULL, NULL, NULL, "PS", "ps_5_0", 0, 0, &pixelShaderBlob, &errorBlob);
    if (FAILED(hr))
    {
        HandleErrorShader(errorBlob);
        return NULL;
    }
    if (errorBlob) errorBlob->Release();

    // Create shaders.
    //ID3D11VertexShader* vertexShader;
    //ID3D11PixelShader* pixelShader;
    Shader* shd = new Shader;

    // Then compile the shaders.
    d3device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), NULL, &shd->vertexShader.vs);
    d3device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), NULL, &shd->pixelShader);

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
        d3device->CreateInputLayout(layout, 7, vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(), &shd->vertexShader.inputLayout);

    }
    else if (format == SHADER_LAYOUT_FLATCOLOR)
    {
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        d3device->CreateInputLayout(layout, 2, vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(), &shd->vertexShader.inputLayout);

    }
    else if (format == SHADER_LAYOUT_BASIC_SHADED)
    {
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
         };
        d3device->CreateInputLayout(layout, 4, vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(), &shd->vertexShader.inputLayout);

        puts("Loaded shader!");
    }
    else
    {
        fprintf(stderr, "%s: Invalid shader layout!\n", __func__);
        _FREE(shd);
        return NULL;
    }

    //immediateContext->IASetInputLayout(shd->vertexShader.inputLayout);

    return shd;

}

static void ShaderTerminate(Shader* shader)
{
    if (shader)
    {
        delete shader;
    }
}

static int ShaderUse(Shader* shader)
{
    if (shader)
    {
        immediateContext->IASetInputLayout(shader->vertexShader.inputLayout);

        immediateContext->VSSetShader(shader->vertexShader.vs, NULL, 0);
        immediateContext->PSSetShader(shader->pixelShader, NULL, 0);
    }
    return -1;
}

static void DoGamma(float* r, float* g, float* b)
{
    *r = 0.75f * (*r * *r) + 0.25f * *r * (*r * *r);
    *g = 0.75f * (*g * *g) + 0.25f * *g * (*g * *g);
    *b = 0.75f * (*b * *b) + 0.25f * *b * (*b * *b);
}

/*
   ...along with the shader uniform constants.
*/

/*
static void CopyLight(DrawPass* pass, ConstantPsLight* dest, Light* l, b32 pointSource)
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
*/

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

    ConstantPsScene* scene = (ConstantPsScene *) mappedBuffer.pData;
    // Copy the lights.
    /*
    CopyLight(pass, &scene->dirLight, pass->dirLight, FALSE);
    for (int i = 0; i < DRAW_MAX_POINTLIGHTS; ++i) {
        CopyLight(pass, &scene->pointLights[i], &pass->pointLights[i], TRUE);
    }
    */

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

    ConstantVS* mats = (ConstantVS *) (matRes.pData);

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

    ConstantPS* ps = (ConstantPS *) (mappedPS.pData);
    for (int i = 0; i < DRAW_MAX_TEX; ++i)
    {
        ps->tex[i].offs[0] = drawState.tex[i].x;
        ps->tex[i].offs[1] = drawState.tex[i].y;
        ps->tex[i].scale[0] = drawState.tex[i].xs;
        ps->tex[i].scale[1] = drawState.tex[i].ys;
    }
    immediateContext->Unmap(constantPSBuffer, 0);

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


    // Then link the constant buffers.
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
    d->y = -pos.y;
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
            immediateContext->OMSetDepthStencilState(depthStencil3DnoWrite, 0);
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
            immediateContext->RSSetState(raster3Dinvert);
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
            immediateContext->OMSetDepthStencilState(depthStencil3DnoWrite, 0);
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
            immediateContext->RSSetState(raster3Dinvert);
            break;
    }
}

// Uploads immutable data.
extern GFX_mesh* DrawUploadMesh(ARENA* arena, void* verts, void* indices, int nVertices)
{
    // TODO: Might be an issue.
    GFX_mesh* m = ArenaPushStruct(arena, GFX_mesh);

    size_t vertexSize = sizeof(CustomVertexDX11);

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = vertexSize * m->nElements;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA subres;
    subres.pSysMem = verts;
    subres.SysMemPitch = vertexSize;
    subres.SysMemSlicePitch = 0;

    ID3D11Buffer* buffer;
    if (FAILED(d3device->CreateBuffer(&bd, NULL, &buffer)))
    {
        // TODO: Temporary arena buffer management.
        fprintf(stderr, "%s: Failed to create buffer!\n", __func__);
        return NULL;
    }
    m->vertexData = buffer;

    bd.ByteWidth = m->nElements * 12;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    subres.pSysMem = indices;
    subres.SysMemPitch = 4; // 4 bytes per integer.
    if (FAILED(d3device->CreateBuffer(&bd, NULL, &buffer)))
    {
        fprintf(stderr, "%s: Failed to create buffer!\n", __func__);
        return NULL;
    }
    m->indexData = buffer;

    m->nElements = nVertices / 3;

    return m;
}

/*
extern void ReleaseMesh(GFX_mesh* m)
{
    ID3D11Buffer* buf;
    buf = reinterpret_cast<ID3D11Buffer *>(m->vertexData);
    buf->Release();
    buf = reinterpret_cast<ID3D11Buffer *>(m->indexData);
    buf->Release();

    //MeshTerminate(m);
}
*/

extern void DrawRenderMesh(GFX_mesh* m)
{
    DrawFlush();
    Draw_SetConstants(&drawState.matStack[drawState.matStackIdx]);

    ID3D11Buffer* verts = reinterpret_cast<ID3D11Buffer *>(m->vertexData);
    ID3D11Buffer* indices = reinterpret_cast<ID3D11Buffer *>(m->indexData);

    UINT stride = sizeof(CustomVertexDX11);
    UINT offset = 0;

    immediateContext->IASetVertexBuffers(0, 1, &verts, &stride, &offset);
    immediateContext->IASetIndexBuffer(indices, DXGI_FORMAT_R32_UINT, 0);
    immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    immediateContext->DrawIndexed(m->nElements * 3, 0, 0);
}

extern void DrawMeshTerminate(GFX_mesh *mesh)
{
    if (mesh == NULL)
    {
        return;
    }

    ID3D11Buffer* buf;
    buf = reinterpret_cast<ID3D11Buffer *>(mesh->vertexData);
    buf->Release();
    buf = reinterpret_cast<ID3D11Buffer *>(mesh->indexData);
    buf->Release();
}


static int CreateTargetSurface(TargetSurface* surface)
{
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

//static void ReleaseTexture(GFX_texture* texture);

static void DeleteTargetSurface(TargetSurface* surface)
{
    surface->depthStencilView->Release();
    ReleaseTexture(&surface->depthStencil);
    surface->colorView->Release();
    ReleaseTexture(&surface->color);
}

static HRESULT InitDirect3D(Window* win)
{
    puts("Initializing DirectX11...");

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

    //fprintf(stderr, "%s: YOO win w:%d, win h:%d\n", __func__, win->w, win->h);
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
    HWND winHandle = *((HWND *) win->user);

    fprintf(stderr, "%s: Win handle: %p\n", __func__, winHandle);
    fprintf(stderr, "%s: window w: %d window h: %d\n", __func__, win->w, win->h);

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
    sd.OutputWindow = winHandle;
    // Turn multisampling off.
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    /*
    if (FULLSCREEN) {
        sd.Windowed = FALSE;
    } else {
        sd.Windowed = TRUE;
    }
    */
    sd.Windowed = TRUE;

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

    bufferDesc.ByteWidth = sizeof(ConstantVS);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = d3device->CreateBuffer(&bufferDesc, NULL, &constantVSBuffer);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to create constant vertex shader buffer!\n", __func__);
        return -1;
    }

    bufferDesc.ByteWidth = sizeof(ConstantPS);
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

    // Init shaders.
    //defaultShader = ShaderLoad(SHADER_LAYOUT_SKINNED_ANIMATION, color_vs, color_ps);

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
    d3device->CreateRasterizerState(&rasterizerDesc, &raster3Dinvert);


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


    drawState.blend = static_cast<BlendMode>(-1);

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
    hr = d3device->CreateDepthStencilState(&depthStencilDesc, &depthStencil3DnoWrite);
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

static DrawPass _passes[1] = { 0 };
static mat4 viewMat = mat4{1.0f};

extern int GraphicsInit(Window* win)
{
    // Create draw arena.
    drawState.drawArena = ArenaInit(MB(4), KB(4), ARENA_FLAG_GROWABLE);

    // Init graphics.
    if (FAILED(InitDirect3D(win)))
    {
        fprintf(stderr, "%s: Failed to initialize Direct3D!\n", __func__);
        return -1;
    }

    DrawSetPipeline(1, _passes);
    DrawSetPass2D(&_passes[0]);
    _passes[0].active = TRUE;
    _passes[0].viewMatrix = &viewMat;
    _passes[0].flags |= DRAW_PASS_FLAG_GAMMA;

    DrawReset();
    DrawSetTarget();

    drawState.currentPass = 0;

    // Init shader and buffer.
    defaultShader = ShaderLoad(SHADER_LAYOUT_BASIC_SHADED, color_vs, color_ps);
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
    depthStencil3DnoWrite->Release();
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
    raster3Dinvert->Release();

    // USER STUFF.

    // Remove basic shader.
    //defaultShader->vertexShader.vs->Release();
    //defaultShader->vertexShader.inputLayout->Release();
    //defaultShader->pixelShader->Release();
    ShaderTerminate(defaultShader);

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

extern void AllocateMutableTexture(int width, int height, void** args, int flags)
{
    /*
    D3DFORMAT format = D3DFMT_X8R8G8B8;
    if (flags & TEXTURE_ALPHA)
    {
        format = D3DFMT_A8R8G8B8;
    }

    // D3DPOOL_DEFAULT is for mutable textures,
    // while D3DPOOL_MANAGED optimizes those textures if they're immutable.
    d3device->CreateTexture(
        width, height, 1, 1, format,
        D3DPOOL_DEFAULT, reinterpret_cast<LPDIRECT3DTEXTURE9*>(args), NULL
    );
    */

    D3D11_TEXTURE2D_DESC textureDesc;
    ZeroMemory(&textureDesc, sizeof(textureDesc));
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = (flags & TEXTURE_SRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    //textureDesc.SampleDesc.Quality = 1;
    textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.MiscFlags = 0;
    // We use mipmapping by default?
    if (flags & TEXTURE_MIPMAP)
    {
        textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.MipLevels = 1;
    }

    D3D11_SUBRESOURCE_DATA textureData;
    textureData.pSysMem = NULL;
    textureData.SysMemPitch = width * 4;

    ID3D11Texture2D* texture2D;
    HRESULT hr = d3device->CreateTexture2D(&textureDesc, &textureData, &texture2D);
    if (FAILED(hr))
    {
        fprintf(stderr, "%s: Failed to load immutable texture!\n", __func__);
        return;
    }

}



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

    //TEXTURE* tex = (TEXTURE *) _MALLOC(sizeof(TEXTURE));
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
    tex->resourceView = resourceView;

    return tex;
}

extern void ReleaseTexture(GFX_texture* texture)
{
    if (texture == NULL)
    {
        return;
    }

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
    }
    else
    {
        delete texture;
    }
}
