#include "draw3d.h"
#include "gfx.h"
//#include "camera.h"
#include "../platform/window.h"

// We use stack allocator.
//#include "../misc/mem_heap.h"
#include "../misc/mem.h"
#include "../misc/arena.h"

#include <math.h>
#include <assert.h>

// We have to init camera.

extern int DrawMatIdentity(void)
{
    drawState.matStack[drawState.matStackIdx] = mat4{ 1.0f };
    drawState.normalMatValid = false;

    return 0;
}
extern int DrawMatTranslate3D(float x, float y, float z)
{
    //fprintf(stderr, "%s: Mat stack index:%d\n", __func__, drawState.matStackIdx);

    drawState.matStack[drawState.matStackIdx].Translate(vec4 { x, y, z, 0});
    return 0;
}

extern int DrawSetMatrix(float* mat)
{
    mat4* m = &drawState.matStack[drawState.matStackIdx];
    for (int i = 0; i < 16; ++i)
    {
        m[i] = mat[i];
    }
    drawState.normalMatValid = false;

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
    DRAWPASS* pass = &drawState.passes[drawState.currentPass];
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
   Line path utilities.
*/


// TODO: Go back to this one.

#define LINE_PATH_CAPACITY 256

/*
extern void LinePathInit(LINEPATH* path)
{
    _MEMSET(path, 0, sizeof(*path));
    path->nPoints = 0;
    path->capacity = LINE_PATH_CAPACITY;
    path->freeCount = LINE_PATH_CAPACITY;
}


extern int LinePathReserve(LINEPATH* path, size_t desired)
{
    if (desired >= path->capacity)
    {
        if (path->capacity == 0) {
            path->capacity = LINE_PATH_CAPACITY;
        }
        while (desired >= path->capacity) {
            path->capacity *= 2;
        }

        path->points = (vec3 *) _REALLOC(path->points, sizeof(vec3) * path->capacity);
        assert(path->points != NULL);

        path->flags = (u32 *) _REALLOC(path->flags, sizeof(u32) * path->capacity);
        assert(path->flags != NULL);

        path->ids = (int *) _REALLOC(path->ids, sizeof(int) * path->capacity);
        assert(path->ids != NULL);

        path->freeList = (int *) _REALLOC(path->freeList, sizeof(int) * path->capacity);
        assert(path->freeList != NULL);

        for (int i = 0; i < path->capacity; ++i)
        {
            path->freeList[i] = i;
        }
        path->freeCount = path->capacity;
    }

    return 0;
}


// Free list allocation.
extern int LinePathAlloc(LINEPATH* path)
{
    if (path->freeCount == 0)
    {
        return -1;
    }

    int index = path->freeList[--path->freeCount];
    path->ids[index] = index;
    return index;
}
extern void LinePathDealloc(LINEPATH* path, int index)
{
    if (index < 0 || index >= path->capacity)
    {
        return; // Invalid index.
    }

    path->freeList[path->freeCount++] = index;
}


extern int LinePathAdd(LINEPATH* path, float x, float y, float z, u32 flags)
{
    LinePathReserve(path, LINE_PATH_CAPACITY);

    // Obtain our index from the object pool.
    int index = LinePathAlloc(path);
    if (index == -1)
    {
        fprintf(stderr, "%s: There's nothing free in the list!\n", __func__);
        return -1;
    }

    vec3* point = &path->points[index];
    point->x = x;
    point->y = y;
    point->z = z;

    u32* flag = &path->flags[index];
    *flag = flags;


    return 0;
}

extern void LinePathSub(LINEPATH* path, int index)
{
    LinePathDealloc(path, index);
}
*/

/*
static LINEPATH_CONTEXT lpc = { 0 };

static void LinePathRendererInit(void)
{
    if (lpc.initialized)
    {
        return;
    }

    _MEMSET(&lpc, 0, sizeof(lpc));
    lpc.frameArena = ArenaInit(MB(4), KB(16), ARENA_FLAG_GROWABLE);
    lpc.nPaths = 0;

    lpc.initialized = TRUE;

    fprintf(stderr, "%s: Initialized line path renderer!\n", __func__);
}

static void LinePathRendererTerminate(void)
{
    if (lpc.initialized)
    {
        return;
    }
    ArenaTerminate(lpc.frameArena);

    fprintf(stderr, "%s: Finished everything!\n", __func__);
}


static void LinePathRendererAddPath(LINEPATH* path)
{
    if (!lpc.initialized)
    {
        LinePathRendererInit();
    }

    lpc.nPaths ++;
}

static void LinePathRendererClear(void)
{
    if (!lpc.initialized)
    {
        LinePathRendererInit();
    }

    ArenaClear(lpc.frameArena);
}

// Allocate int arena allocator.
extern int LinePathInit(LINEPATH* path)
{
    if (path->allocated)
    {
        return 0;
    }
    LinePathRendererInit();

    _MEMSET(path, 0, sizeof(*path));

    size_t len = sizeof(vec2) * LINE_PATH_CAPACITY;
    ARENA* arena = lpc.frameArena;
    void* mem = ArenaPush(arena, len, 0); // Push zero memory.
    PoolInitEx(&path->pool, mem, len, sizeof(vec2), 8);

    path->points = (vec2 *) mem;
    path->nPoints = 0;
    path->allocated = TRUE;

    LinePathRendererAddPath(path);

    return 0;
}

extern void LinePathTerminate(LINEPATH *path)
{
    if (path->allocated)
    {
        PoolTerminate(&path->pool);
    }
}


extern vec2* LinePathAdd(LINEPATH* path, float x, float y)
{
    if (!path->allocated)
    {
        // Lazy initialization.
        LinePathInit(path);
    }

    vec2* point = (vec2 *) PoolAlloc(&path->pool);
    point->x = x;
    point->y = y;

    path->nPoints++;

    return point;
}

extern void LinePathSub(LINEPATH* path, vec2* ptr)
{
    if (!path->allocated || path->nPoints <= 0)
    {
        return;
    }
    PoolFree(&path->pool, ptr);
    path->nPoints--;
}
*/


// Geometry.
extern void LinePathCreateStar(
    float* path, int pathLen,
    int points, float rotation, float r1, float r2)
{
    if (path == NULL || pathLen < points*4)
    {
        return;
    }

    float sideAngle = PI2 / (float) points;
    float offset = sideAngle * 0.5f;

    float theta = rotation;
    for (int i = 0; i < points; ++i)
    {
        float x1 = cosf(theta) * r1;
        float y1 = sinf(theta) * r1;
        float x2 = cosf(theta + offset) * r2;
        float y2 = sinf(theta + offset) * r2;

        path[i*4 + 0] = x1;
        path[i*4 + 1] = y1;
        path[i*4 + 2] = x2;
        path[i*4 + 3] = y2;

        theta += sideAngle;
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


    float w2 = w / 2.0f, h2 = h / 2.0f;

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

    float w2 = w / 2.0f, h2 = h / 2.0f;
    DrawVertex(rx * -w2 + ux * -h2, ry * -w2 + uy * -h2, rz * -w2 + uz * -h2, drawState.srcX, drawState.srcY, col[0][0], col[0][1], col[0][2], col[0][3]);
    DrawVertex(rx * +w2 + ux * -h2, ry * +w2 + uy * -h2, rz * +w2 + uz * -h2, drawState.srcX + drawState.srcW, drawState.srcY, col[1][0], col[1][1], col[1][2], col[1][3]);
    DrawVertex(rx * -w2 + ux * +h2, ry * -w2 + uy * +h2, rz * -w2 + uz * +h2, drawState.srcX, drawState.srcY + drawState.srcH, col[2][0], col[2][1], col[2][2], col[2][3]);
    DrawVertex(rx * +w2 + ux * +h2, ry * +w2 + uy * +h2, rz * +w2 + uz * +h2, drawState.srcX + drawState.srcH, drawState.srcY + drawState.srcH, col[3][0], col[3][1], col[3][2], col[3][3]);

    const u32 indices[6] = { 2, 3, 0, 3, 1, 0 };
    DrawIndices(4, 6, indices);
}

extern void DrawEllipse(int npoints, f32 w, f32 h)
{
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

    u32* indices = (u32 *) StackDoAlloc(npoints * 3 * sizeof(u32));

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

    // TODO: Memory allocation.
    StackDoDealloc(indices);
}

extern void DrawArcSector(int npoints, f32 rStart, f32 r, f32 w1, f32 w2)
{
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

    size_t isize = npoints1 * 6 * sizeof(int);

    u32* indices = (u32 *) StackDoAlloc(isize);

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
    StackDoDealloc(indices);
}



extern void DrawLine(float x0, float y0, float x1, float y1, float thickness)
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

    float cx = (x0 + x1) / 2.0f;
    float cy = (y0 + y1) / 2.0f;
    float dx = (x1 - x0);
    float dy = (y1 - y0);

    float ang = atan2f(dy, dx);
    float len = sqrtf(dx * dx + dy * dy);

    float c = cosf(ang);
    float s = sinf(ang);

    float hw = len / 2.0f;
    float hh = thickness;

    // The offsets.
    float px0 = -hw*c + hh*s;
    float py0 = -hw*s - hh*c;
    float px1 = +hw*c + hh*s;
    float py1 = +hw*s - hh*c;
    float px2 = -hw*c - hh*s;
    float py2 = -hw*s + hh*c;
    float px3 = +hw*c - hh*s;
    float py3 = +hw*s + hh*c;

    // Multiplying rotation matrix.
    /*
       [cos(t), -sin(t) ] [x] = [cos(t) * x - sin(t) * y]
       [sin(t), cos(t)] [y] = [sin(t) * x + cos(t) * y]
    */

    DrawPreFlush(4, 6);

    /*
    DrawVertex(-w2, -h2, 0, drawState.srcX, drawState.srcY, col[0][0], col[0][1], col[0][2], col[0][3]);
    DrawVertex(+w2, -h2, 0, drawState.srcX + drawState.srcW, drawState.srcY, col[1][0], col[1][1], col[1][2], col[1][3]);
    DrawVertex(-w2, +h2, 0, drawState.srcX, drawState.srcY + drawState.srcH, col[2][0], col[2][1], col[2][2], col[2][3]);
    DrawVertex(+w2, +h2, 0, drawState.srcX + drawState.srcW, drawState.srcY + drawState.srcH, col[3][0], col[3][1], col[3][2], col[3][3]);
    */

    DrawVertex(px0 + cx, py0 + cy, 0, drawState.srcX, drawState.srcY, col[0][0], col[0][1], col[0][2], col[0][3]);
    DrawVertex(px1 + cx, py1 + cy, 0, drawState.srcX + drawState.srcW, drawState.srcY, col[1][0], col[1][1], col[1][2], col[1][3]);
    DrawVertex(px2 + cx, py2 + cy, 0, drawState.srcX, drawState.srcY + drawState.srcH, col[2][0], col[2][1], col[2][2], col[2][3]);
    DrawVertex(px3 + cx, py3 + cy, 0, drawState.srcX + drawState.srcW, drawState.srcY + drawState.srcH, col[3][0], col[3][1], col[3][2], col[3][3]);


    const u32 indices[6] = { 2, 3, 0, 3, 1, 0 };
    DrawIndices(4, 6, indices);
}

extern void DrawLinePathEx(float* pointData, int nPoints, float thickness, u32 flags)
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

    int closed = (flags & LINE_PATH_CLOSED);
    int nPoints1 = nPoints - !closed;


    // TODO: This might fail because vec2 is 16 bytes long.
    //vec2* points = (vec2 *) pointData;
    float* points = (float *) pointData;
    size_t isize = nPoints * 6 * sizeof(int);

    u32* indices = (u32 *) StackDoAlloc(isize);

    vec2 ab, bc, offset;
    for (int i = 0; i < nPoints; ++i)
    {
        int ia = ((i-1)%nPoints)*2;
        int ib = i*2;
        int ic = ((i+1)%nPoints)*2;

        float ax = points[ia];
        float ay = points[ia+1];
        float bx = points[ib];
        float by = points[ib+1];
        float cx = points[ic];
        float cy = points[ic+1];

        vec2 a { ax, ay };
        vec2 b { bx, by };
        vec2 c { cx, cy };

        Vec2Sub(&ab, &a, &b);
        Vec2Sub(&bc, &b, &c);
        Vec2Add(&offset, &ab, &bc);
        Vec2Nor2(&offset, &offset);
        Vec2MulScalar(&offset, &offset, thickness);

        // Inner.
        DrawVertex(a.x + offset.x, a.y + offset.y, 0, drawState.srcX, drawState.srcY, col[0][0], col[0][1], col[0][2], col[0][3]);
        // Outer.
        DrawVertex(a.x - offset.x, a.y - offset.y, 0, drawState.srcX + drawState.srcW, drawState.srcY + drawState.srcH, col[1][0], col[1][1], col[1][2], col[1][3]);

        if (i != nPoints1)
        {
            // Make rectangles.
            indices[i*6+0] = (i*2) + 1;
            indices[i*6+1] = (i*2) + 0;
            indices[i*6+2] = (i*2) + 2;
            indices[i*6+3] = (i*2) + 3;
            indices[i*6+4] = (i*2) + 1;
            indices[i*6+5] = (i*2) + 2;
        }

    }

    DrawIndices(nPoints * 2, nPoints1 * 6, indices);
    StackDoDealloc(indices);
}

extern void DrawLinePath(LINEPATH* path, float thickness, u32 flags);



extern void DrawSkybox(const char* path)
{
    DrawVertex(-1, -1, -1, 0, 0, 0,  0, 0, 1);
    DrawVertex(+1, -1, -1, 0, 0, 0,  0, 0, 1);
    DrawVertex(-1, +1, -1, 0, 0, 0,  0, 0, 1);
    DrawVertex(+1, +1, -1, 0, 0, 0,  0, 0, 1);
    DrawVertex(-1, -1, +1, 0, 0, 0,  0, 0, 1);
    DrawVertex(+1, -1, +1, 0, 0, 0,  0, 0, 1);
    DrawVertex(-1, +1, +1, 0, 0, 0,  0, 0, 1);
    DrawVertex(+1, +1, +1, 0, 0, 0,  0, 0, 1);

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
   Pre-drawing.
*/

extern void DrawSrcRect(float x, float y, float w, float h)
{
    if (drawState.nTex && drawState.tex[0].tex)
    {
        TEXTURE* t = drawState.tex[0].tex;
        drawState.srcX = (float) x / t->w;
        drawState.srcY = (float) y / t->h;
        drawState.srcW = (float) w / t->w;
        drawState.srcH = (float) h / t->h;

        //fprintf(stderr, "%s: We have a texture!\n", __func__);
    }
    else
    {
        drawState.srcX = x;
        drawState.srcY = y;
        drawState.srcW = w;
        drawState.srcH = h;
        //fprintf(stderr, "%s: Greg\n", __func__);
    }
    //fprintf(stderr, "%s: Drawing src rect!\n", __func__);
}



extern void DrawColorMode(COLORMODE mode)
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
   // fprintf(stderr, "%s: Resetting!\n", __func__);
    DrawFlush();

    //_MEMSET(&drawState, 0, sizeof(DRAWSTATE));

    drawState.matStackIdx = 0;
    DrawMatIdentity();

    // TODO: Use shader.
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


    /*
DrawBlend(BLEND_ALPHA);
    drawState.nTex = 0;

    DrawSrcRect(0, 0, 1, 1);
    DrawColor1(1, 1, 1, 1);
    DrawColor2(1, 1, 1, 1);
    DrawColorMode(COLOR1);
    DrawZBufWrite(TRUE);
    DrawUVModelMat(FALSE);
    DrawWireframe(FALSE);
    DrawCullInvert(FALSE);
    */
}

extern void DrawEnd(void)
{
    // Clear frame arena.
    //LinePathRendererClear();

    DrawGfxEnd();
}


extern void DrawClearLights(void)
{
    //_MEMSET(&dirLight, 0, sizeof(dirLight));
    //_MEMSET(&pointLights[0], 0, sizeof(LIGHT) * DRAW_MAX_POINTLIGHTS);
}

extern void SetLight(LIGHT *l, f32 x, f32 y, f32 z,
    u32 color, f32 ambient, f32 diffuse, f32 specular,
    f32 linear, f32 quadratic, f32 intensity)
{
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;

    l->x = x;
    l->y = y;
    l->z = z;
    l->ambR = r*ambient*intensity;
    l->ambG = g*ambient*intensity;
    l->ambB = b*ambient*intensity;

    l->difR = r*diffuse*intensity;
    l->difG = g*diffuse*intensity;
    l->difB = b*diffuse*intensity;

    l->spcR = r*specular*intensity;
    l->spcG = g*specular*intensity;
    l->spcB = b*specular*intensity;

    l->constant = 1.0f;
    l->linear = linear;
    l->quadratic = quadratic; // Realistic attenuation for point light.
}

extern int DrawInit(void)
{
    // Init memory I think.
    MemoryInit();

    //Camera3Init(&camera3Handle);
    // Son...
    GraphicsDriverInit(L"Window がいせき", 800, 600);

    return 0;
}

extern void DrawTerminate(void)
{
    //LinePathRendererTerminate();

    GraphicsTerminate();
    WindowTerminate(windowHandle);

    MemoryTerminate();
}


/*
   Draw passes.
*/

extern int DrawSetPipeline(int nPasses, DRAWPASS *passes)
{
    drawState.nPasses = nPasses;
    drawState.passes = passes;

    return 0;
}

extern int DrawSetPass2D(DRAWPASS* pass)
{
    _MEMSET(pass, 0, sizeof(*pass));
    pass->target = -1;
    pass->stdShader = SHADER_2D;
    pass->viewportW = windowHandle->w;
    pass->viewportH = windowHandle->h;
    pass->samplerMode = SAMPLER_LINEAR;
    pass->cullMode = CULL_NONE;
    pass->fogMin = 4096.0f;
    pass->fogMax = 8192.0f;
    pass->fogColor = 0xffffffff;

    return 0;
}

extern int DrawSetPass3D(DRAWPASS* pass)
{
    _MEMSET(pass, 0, sizeof(*pass));
    pass->target = -1;
    pass->stdShader = SHADER_3D;
    pass->viewportW = windowHandle->w;
    pass->viewportH = windowHandle->h;

    pass->samplerMode = SAMPLER_LINEAR;
    pass->cullMode = CULL_FRONT;
    pass->depthStencilMode = DEPTH_STENCIL_DEPTH;

    pass->projectionMode = PROJECTION_PERSPECTIVE;
    pass->proj.perspective.fovy = RADIANS(90);
    pass->proj.perspective.fr = 1000.0f;
    pass->proj.perspective.nr = 0.1f;
    pass->flags = DRAW_PASS_FLAG_3D;

    pass->fogMin = 4096.0f;
    pass->fogMax = 8192.0f;
    pass->fogColor = 0xffffffff;

    return 0;
}


extern int DrawFullFrame(void)
{
    drawState.totalFlushes = 0;
    drawState.currentPass = 0;
    for (; drawState.currentPass < drawState.nPasses;
        ++drawState.currentPass)
    {
        DRAWPASS* pass = &drawState.passes[drawState.currentPass];
        if (pass->active)
        {
            DrawReset();
            drawState.zWrite = (pass->depthStencilMode == DEPTH_STENCIL_DEPTH);
            DrawSetTarget();
            //DrawUpdateFrustum();
            //pass->draw(pass);
            //DrawFlush();

            //fprintf(stderr, "%s: Draw shi\n", __func__);
            // Do some flushing afterwards.
        }
    }
    return 0;
}

/*
Using:
    DRAWPASS pass;
    DrawSetPass3D(&pass);
    pass.draw = SceneDrawingFunction;
*/
