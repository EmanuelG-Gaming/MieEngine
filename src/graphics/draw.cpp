#include "draw.h"
#include "../base/math/mathf.h"
#include "../base/math/mathf_wrap.h"

#include <string.h>
#include <math.h>

#define ROTATION_3D 1


/*
   Matrices.
*/

extern int DrawMatIdentity(void)
{
    drawState.matStack[drawState.matStackIdx] = mat4{ 1.0f };
    drawState.normalMatValid = FALSE;

    return 0;
}
extern int DrawMatTranslate3D(float x, float y, float z)
{
    drawState.matStack[drawState.matStackIdx].Translate(vec4{ x, y, z, 0 }); 
    return 0;
}

extern int DrawMatScale3D(float x, float y, float z)
{
    drawState.matStack[drawState.matStackIdx].Scale(vec4 { x, y, z, 1.0f });
    return 0;
}


// Rotate on Y axis.
extern int DrawMatRotateYA(float rad)
{
    if (ROTATION_3D)
    {
        rad = -rad;
    }
    float c = cosf(rad), s = sinf(rad);

    mat4 m { 1.0f };
    m.c[0] = c;
    m.c[2] = -s;
    m.c[8] = s;
    m.c[10] = c;

    drawState.matStack[drawState.matStackIdx] = drawState.matStack[drawState.matStackIdx] * m;

    //drawState.matStack[drawState.matStackIdx].Rotate({ 0, rad, 0, 0 });
    drawState.normalMatValid = FALSE;

    return 0;
}
extern int DrawMatRotate3D(float x, float y, float z)
{
    float r[3];
    if (ROTATION_3D)
    {
        r[0] = -x;
        r[1] = -y;
        r[2] = -z;
    }
    else
    {
        r[0] = x;
        r[1] = y;
        r[2] = z;
    }

    drawState.matStack[drawState.matStackIdx].Rotate(vec4::EulerAngles(r[0], r[1], r[2]));
    drawState.normalMatValid = FALSE;

    return 0;
}

extern int DrawSetMatrix(float* mat)
{
    mat4* m = &drawState.matStack[drawState.matStackIdx];
    for (int i = 0; i < 16; ++i)
    {
        m[i] = mat[i];
    }
    drawState.normalMatValid = FALSE;

    return 0;
}

extern int DrawMulMatrix(mat4* mat)
{
    mat4* m = &drawState.matStack[drawState.matStackIdx];
    Mat4Mul(m, m, mat);
    drawState.normalMatValid = FALSE;

    return 0;
}



extern void DrawPushMat(void)
{
    drawState.matStackIdx += 1;
    drawState.matStack[drawState.matStackIdx] = drawState.matStack[drawState.matStackIdx - 1];
}
extern void DrawPopMat(void)
{
    drawState.matStackIdx -= 1;
    drawState.normalMatValid = FALSE;
}

extern void DrawGetProjection(mat4 *out)
{
    DrawPass* pass = &drawState.passes[drawState.currentPass];
    switch (pass->projectionMode)
    {
        case PROJECTION_IDENT:
        default:
            Mat4Ident(out, 1.0f);
            break;

        case PROJECTION_PERSPECTIVE:
            *out = mat4::Perspective(pass->proj.perspective.fovy,
                (float) pass->viewportW / pass->viewportH, pass->proj.perspective.nr, pass->proj.perspective.fr);
            break;

        case PROJECTION_ORTHO:
            *out = mat4::Ortho(pass->proj.ortho.left, pass->proj.ortho.right,
                pass->proj.ortho.top, pass->proj.ortho.bottom,
                pass->proj.ortho.nr, pass->proj.ortho.fr);
            break;

        case PROJECTION_CUSTOM:
            // Copying.
            *out = *pass->proj.custom;
            break;
    }
}


/*
   Drawing routines.
*/


extern void DrawRect(f32 w, f32 h)
{
    float* col[4];
    switch (drawState.colorMode)
    {
        default:
        case COLOR1:
            col[0] = drawState.col1;
            col[1] = drawState.col1;
            col[2] = drawState.col1;
            col[3] = drawState.col1;
            break;
        case COLOR2:
            col[0] = drawState.col2;
            col[1] = drawState.col2;
            col[2] = drawState.col2;
            col[3] = drawState.col2;
            break;
        case COLOR_LR:
            col[0] = drawState.col1;
            col[1] = drawState.col2;
            col[2] = drawState.col1;
            col[3] = drawState.col2;
            break;
        case COLOR_UD:
            col[0] = drawState.col1;
            col[1] = drawState.col1;
            col[2] = drawState.col2;
            col[3] = drawState.col2;
            break;
    }


    DrawPreFlush(4, 6);


    float w2 = w / 2.0f, h2 = -h / 2.0f;

    DrawVertex(-w2, -h2, 0, drawState.srcX, drawState.srcY, col[0][0], col[0][1], col[0][2], col[0][3]);
    DrawVertex(+w2, -h2, 0, drawState.srcX + drawState.srcW, drawState.srcY, col[1][0], col[1][1], col[1][2], col[1][3]);
    DrawVertex(-w2, +h2, 0, drawState.srcX, drawState.srcY + drawState.srcH, col[2][0], col[2][1], col[2][2], col[2][3]);
    DrawVertex(+w2, +h2, 0, drawState.srcX + drawState.srcW, drawState.srcY + drawState.srcH, col[3][0], col[3][1], col[3][2], col[3][3]);

    const u32 indices[6] = { 2, 3, 0, 3, 1, 0 };
    // TODO: maybe this was the issue.
    DrawIndices(4, 6, indices);
}

extern void DrawRectBillboard(f32 w, f32 h)
{
    float* col[4];
    switch (drawState.colorMode)
    {
        default:
        case COLOR1:
            col[0] = drawState.col1;
            col[1] = drawState.col1;
            col[2] = drawState.col1;
            col[3] = drawState.col1;
            break;
        case COLOR2:
            col[0] = drawState.col2;
            col[1] = drawState.col2;
            col[2] = drawState.col2;
            col[3] = drawState.col2;
            break;
        case COLOR_LR:
            col[0] = drawState.col1;
            col[1] = drawState.col2;
            col[2] = drawState.col1;
            col[3] = drawState.col2;
            break;
        case COLOR_UD:
            col[0] = drawState.col1;
            col[1] = drawState.col1;
            col[2] = drawState.col2;
            col[3] = drawState.col2;
            break;
    }

    DrawPreFlush(4, 6);

    mat4* camMat = drawState.passes[drawState.currentPass].viewMatrix;

    float rx = camMat->c[0];
    float ry = camMat->c[4];
    float rz = camMat->c[8];

    float ux = camMat->c[1];
    float uy = camMat->c[5];
    float uz = camMat->c[9];

    float w2 = w / 2.0f, h2 = -h / 2.0f;
    DrawVertex(rx * -w2 + ux * -h2, ry * -w2 + uy * -h2, rz * -w2 + uz * -h2, drawState.srcX, drawState.srcY, col[0][0], col[0][1], col[0][2], col[0][3]);
    DrawVertex(rx * +w2 + ux * -h2, ry * +w2 + uy * -h2, rz * +w2 + uz * -h2, drawState.srcX + drawState.srcW, drawState.srcY, col[1][0], col[1][1], col[1][2], col[1][3]);
    DrawVertex(rx * -w2 + ux * +h2, ry * -w2 + uy * +h2, rz * -w2 + uz * +h2, drawState.srcX, drawState.srcY + drawState.srcH, col[2][0], col[2][1], col[2][2], col[2][3]);
    DrawVertex(rx * +w2 + ux * +h2, ry * +w2 + uy * +h2, rz * +w2 + uz * +h2, drawState.srcX + drawState.srcH, drawState.srcY + drawState.srcH, col[3][0], col[3][1], col[3][2], col[3][3]);

    const u32 indices[6] = { 2, 3, 0, 3, 1, 0 };
    DrawIndices(4, 6, indices);
}


extern void DrawEllipse(int npoints, f32 w, f32 h)
{
    /*
    // Stack memory.
    float* col[2];
    switch (drawState.colorMode)
    {
        default:
        case COLOR1:
            col[0] = drawState.col1;
            col[1] = drawState.col1;
            break;
        case COLOR2:
            col[0] = drawState.col2;
            col[1] = drawState.col2;
            break;
        case COLOR_INOUT:
            col[0] = drawState.col1;
            col[1] = drawState.col2;
            break;
    }

    float w2 = w / 2.0f, h2 = h / 2.0f;
    float sw2 = drawState.srcW / 2.0f, sh2 = drawState.srcH / 2.0f;
    float smx = drawState.srcX + sw2, smy = drawState.srcY + sh2;

    ARENA_TEMP tmp = ArenaTempBegin(drawState.drawArena);
    u32* indices = (u32 *) ArenaPushArrayZero(tmp.arena, u32, npoints * 3);

    DrawVertex(0, 0, 0, drawState.srcX + sw2, drawState.srcY + sh2, col[0][0], col[0][1], col[0][2], col[0][3]);

    float ang = PI/2.0f;
    for (size_t i = 0; i < npoints; ++i)
    {
        float s = sinf(ang), c = cosf(ang);
        DrawVertex(c * w2, s * h2, 0, c*sw2 + smx, s*sh2 + smy, col[1][0], col[1][1], col[1][2], col[1][3]);

        indices[i*3+0] = i + 1;
        indices[i*3+1] = i == (npoints - 1) ? 1 : i +2;
        indices[i*3+2] = 0;

        ang += PI2 / npoints;
    }

    DrawIndices(npoints + 1, npoints * 3, indices);

    ArenaTempEnd(tmp);
    */
}

extern void DrawArcSector(int npoints, f32 rStart, f32 r, f32 w1, f32 w2)
{
    /*
    if (npoints < 2)
    {
        return;
    }

    float* col[2];
    switch (drawState.colorMode)
    {
        default:
        case COLOR1:
            col[0] = drawState.col1;
            col[1] = drawState.col1;
            break;
        case COLOR2:
            col[0] = drawState.col2;
            col[1] = drawState.col2;
            break;
        case COLOR_INOUT:
            col[0] = drawState.col1;
            col[1] = drawState.col2;
            break;
    }

    float rr, ri, rdr, rdi;

    int npoints1 = npoints - 1;
    ANGLE2C(rr, ri, rStart);
    ANGLE2C(rdr, rdi, r / npoints1);

    //size_t isize = npoints1 * 6 * sizeof(int);

    //u32* indices = (u32 *) StackDoAlloc(isize);
    ARENA_TEMP arenaTemp = ArenaTempBegin(drawState.drawArena);
    u32* indices = ArenaPushArrayZero(arenaTemp.arena, u32, npoints1 * 6);

    for (size_t i = 0; i < npoints; ++i)
    {
        float v = LERP(drawState.srcY, drawState.srcY + drawState.srcH, (float) i / npoints1);

        // Inner.
        DrawVertex(rr*w1, ri*w1, 0, drawState.srcX, v, col[0][0], col[0][1], col[0][2], col[0][3]);
        // Outer.
        DrawVertex(rr*(w1 + w2), ri*(w1 + w2), 0, drawState.srcX + drawState.srcW, v, col[1][0], col[1][1], col[1][2], col[1][3]);

        if (i != npoints1)
        {
            // Make rectangles.
            indices[i*6+0] = (i*2) + 1;
            indices[i*6+1] = (i*2) + 0;
            indices[i*6+2] = (i*2) + 2;
            indices[i*6+3] = (i*2) + 3;
            indices[i*6+4] = (i*2) + 1;
            indices[i*6+5] = (i*2) + 2;
        }

        CMUL(rr, ri, rr, ri, rdr, rdi);
    }

    DrawIndices(npoints * 2, npoints1 * 6, indices);
    ArenaTempEnd(arenaTemp);
    */
}


extern void DrawFillStar(int npoints, f32 r1, f32 r2)
{
    /*
    // Stack memory.
    float* col[2];
    switch (drawState.colorMode)
    {
        default:
        case COLOR1:
            col[0] = drawState.col1;
            col[1] = drawState.col1;
            break;
        case COLOR2:
            col[0] = drawState.col2;
            col[1] = drawState.col2;
            break;
        case COLOR_INOUT:
            col[0] = drawState.col1;
            col[1] = drawState.col2;
            break;
    }

    float sw2 = drawState.srcW / 2.0f, sh2 = drawState.srcH / 2.0f;
    float smx = drawState.srcX + sw2, smy = drawState.srcY + sh2;

    ARENA_TEMP tmp = ArenaTempBegin(drawState.drawArena);
    u32* indices = (u32 *) ArenaPushArrayZero(tmp.arena, u32, npoints * 3);

    DrawVertex(0, 0, 0, drawState.srcX + sw2, drawState.srcY + sh2, col[0][0], col[0][1], col[0][2], col[0][3]);

    float ang = PI/2.0f+PI/4.0f;
    float s, c;
    // Inner circle (rad1).
    for (int i = 0; i < npoints; ++i)
    {
        s = sinf(ang), c = cosf(ang);

        ang += PI/4.0f;
    }
    ang = PI/2.0f;

    // Outer circle (rad1 + rad2).
    for (int i = 0; i < npoints; ++i)
    {
        s = sinf(ang), c = cosf(ang);

    }
    ang = PI/2.0f;

    for (int i = 0; i < npoints; ++i)
    {
        float s = sinf(ang), c = cosf(ang);
        DrawVertex(c, s, 0, c*sw2 + smx, s*sh2 + smy, col[1][0], col[1][1], col[1][2], col[1][3]);

        indices[i*3+0] = i + 1;
        indices[i*3+1] = i == (npoints - 1) ? 1 : i + 2;
        indices[i*3+2] = 0;

        ang += PI2 / npoints;
    }

    DrawIndices(npoints + 1, npoints * 3, indices);

    ArenaTempEnd(tmp);
    */
}
extern void DrawLineStar(int npoints, f32 r1, f32 r2, f32 thickness)
{
}
extern void DrawLinePolygram(int npoints, int offset, f32 r1, f32 r2, f32 thickness)
{
}


extern void DrawSkybox(void)
{
    DrawVertex(-1, -1, -1, 1, 0, 0,  0, 0, 0);
    DrawVertex(+1, -1, -1, 1, 0, 0,  0, 1, 0);
    DrawVertex(-1, +1, -1, 1, 0, 0,  0, 1, 1);
    DrawVertex(+1, +1, -1, 1, 0, 0,  0, 0, 1);
    DrawVertex(-1, -1, +1, 1, 0, 0,  0, 0, 0);
    DrawVertex(+1, -1, +1, 1, 0, 0,  0, 1, 0);
    DrawVertex(-1, +1, +1, 1, 0, 0,  0, 1, 1);
    DrawVertex(+1, +1, +1, 1, 0, 0,  0, 0, 1);

    const u32 indices[36] = {
        2,6,7, 2,3,7,
        0,4,5, 0,1,5,
        0,2,6, 0,4,6,
        1,3,7, 1,5,7,
        0,2,3, 0,1,3,
        4,6,7, 4,5,7,
    };

    DrawIndices(8, 36, indices);
}

/*
   State.
*/

// Used by other files.

extern void DrawSrcRect(float x, float y, float w, float h)
{
    if (drawState.nTex && drawState.tex[0].tex)
    {
        // Texture coordinate normalization.
        GFX_texture* t = drawState.tex[0].tex;
        drawState.srcX = (float) x / t->w;
        drawState.srcY = (float) y / t->h;
        drawState.srcW = (float) w / t->w;
        drawState.srcH = (float) h / t->h;
    }
    else
    {
        // Freestanding (No texture used).
        drawState.srcX = x;
        drawState.srcY = y;
        drawState.srcW = w;
        drawState.srcH = h;
    }
}

extern void DrawColorMode(ColorMode mode)
{
    drawState.colorMode = mode;
}
extern void DrawColor(float r, float g, float b, float a)
{
    drawState.col1[0] = r;
    drawState.col1[1] = g;
    drawState.col1[2] = b;
    drawState.col1[3] = a;
}
extern void DrawColor2(float r, float g, float b, float a)
{
    drawState.col2[0] = r;
    drawState.col2[1] = g;
    drawState.col2[2] = b;
    drawState.col2[3] = a;
}

extern void DrawReset(void)
{
    DrawFlush();

    drawState.matStackIdx = 0;
    DrawMatIdentity();

    // Reset textures.
    for (int i = 0; i < drawState.nTex; ++i)
    {
        DrawTextureOffsetScale(i, 0, 0, 1, 1);
    }

    DrawBlend(BLEND_ALPHA);
    drawState.nTex = 0;

    DrawSrcRect(0, 0, 1, 1);
    DrawColor(1, 1, 1, 1);
    DrawColor2(1, 1, 1, 1);
    DrawColorMode(COLOR1);
    DrawZBufferWrite(TRUE);
    DrawUVModelMat(FALSE);
    DrawWireframe(FALSE);
    DrawCullInvert(FALSE);
}


/*
   Draw passes.
*/

extern void DrawSetPipeline(int nPasses, DrawPass* passes)
{
    drawState.nPasses = nPasses;
    drawState.passes = passes;
}
extern void DrawSetPass2D(DrawPass* pass)
{
    _MEMSET(pass, 0, sizeof(*pass));
    pass->target = -1;
    //pass->stdShader = SHADER_2D;
    pass->viewportW = windowHandle->w;
    pass->viewportH = windowHandle->h;
    pass->samplerMode = SAMPLER_LINEAR;
    pass->cullMode = CULL_NONE;
    pass->fogMin = 4096.0f;
    pass->fogMax = 8192.0f;
    pass->fogColor = 0xffffffff;
}
extern void DrawSetPass3D(DrawPass* pass)
{
    _MEMSET(pass, 0, sizeof(*pass));

    pass->target = -1;
    //pass->stdShader = SHADER_3D;
    pass->viewportW = windowHandle->w;
    pass->viewportH = windowHandle->h;

    pass->samplerMode = SAMPLER_LINEAR;
    //pass->cullMode = CULL_FRONT;
    pass->cullMode = CULL_NONE;
    pass->depthStencilMode = DEPTH_STENCIL_DEPTH;

    pass->projectionMode = PROJECTION_PERSPECTIVE;
    pass->proj.perspective.fovy = RADIANS(90);
    pass->proj.perspective.fr = 1000.0f;
    pass->proj.perspective.nr = 0.1f;
    pass->flags = DRAW_PASS_FLAG_3D;

    pass->fogMin = 4096.0f;
    pass->fogMax = 8192.0f;
    pass->fogColor = 0xffffffff;
}

extern int DrawFullFrame(void)
{
    drawState.totalFlushes = 0;
    drawState.currentPass = 0;
    for (; drawState.currentPass < drawState.nPasses;
        ++drawState.currentPass)
    {
        DrawPass* pass = &drawState.passes[drawState.currentPass];
        if (pass->active)
        {
            DrawReset();
            drawState.zWrite = (pass->depthStencilMode == DEPTH_STENCIL_DEPTH);
            DrawSetTarget();
        }
    }
    return 0;
}


//DrawUpdateFrustum();
//pass->draw(pass);
//DrawFlush();

//fprintf(stderr, "%s: Draw shi\n", __func__);
// Do some flushing afterwards.
