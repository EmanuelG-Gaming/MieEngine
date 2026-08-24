#include "truetype_renderer.h"
#include "truetype_parse.h"

// Needs math library, though.
#include "../base/mathf.h"
#include "truetype.h"

#include <math.h>
#include <string.h>
#include <assert.h>

#define TT_CORNER_DOT_THRESH 0.99f

// CMY colors.
#define TT_WHITE (TT_POINT_FLAG_RED | TT_POINT_FLAG_GREEN | TT_POINT_FLAG_BLUE)
#define TT_CYAN (TT_POINT_FLAG_GREEN | TT_POINT_FLAG_BLUE)
#define TT_MAGENTA (TT_POINT_FLAG_RED | TT_POINT_FLAG_BLUE)
#define TT_YELLOW (TT_POINT_FLAG_RED | TT_POINT_FLAG_GREEN)

static inline int Equal(float* a, float* b)
{
    return ((a[0] == b[0]) && (a[1] == b[1]));
}


// Edge coloring (for debugging).
extern void TT_GlyphColorEdges(TT_GLYPHDATA* glyph)
{
    u32 index = 0;
    for (int contour = 0; contour < glyph->nConturs; ++contour)
    {
        u32 numCorners = 0;
        u32 contourStart = index;
        u32 edgeStart = index;

        u32 color = TT_MAGENTA;

        vec2 firstStartDir = { 1, 0 };
        vec2 prevEndDir = { 1, 0 };

        while (
            index < glyph->nPoints &&
            (glyph->flags[index] & TT_POINT_FLAG_CONTOUR_END) != TT_POINT_FLAG_CONTOUR_END
        )
        {
            b32 firstSegment = index == contourStart;

            u32 numPoints = 0;
            vec2 points[3] = { 0 };

            if (glyph->flags[index] & TT_POINT_FLAG_LINE)
            {
                vec2i16 p0 = glyph->points[index++];
                vec2i16 p1 = glyph->points[index];

                numPoints = 2;
                points[0] = CLITERAL(vec2) { (float) p0.x, (float) -p0.y };
                points[1] = CLITERAL(vec2) { (float) p1.x, (float) -p1.y };
            }
            else
            {
                vec2i16 p0 = glyph->points[index++];
                vec2i16 p1 = glyph->points[index++];
                vec2i16 p2 = glyph->points[index];

                numPoints = 3;
                points[0] = CLITERAL(vec2) { (float) p0.x, (float) -p0.y };
                points[1] = CLITERAL(vec2) { (float) p1.x, (float) -p1.y };
                points[2] = CLITERAL(vec2) { (float) p2.x, (float) -p2.y };
            }

            vec2 startDir; Vec2Sub(&startDir, &points[1], &points[0]);
            Vec2Nor2(&startDir, &startDir);

            if (firstSegment)
            {
                firstStartDir = startDir;
            }
            else if (Vec2Dot2(&prevEndDir, &startDir) < TT_CORNER_DOT_THRESH)
            {
                if (color == TT_YELLOW) {
                    color = TT_CYAN;
                } else {
                    color = TT_YELLOW;
                }

                numCorners ++;
                edgeStart = index + 1 - numPoints;
            }

            glyph->flags[index + 1 - numPoints] |= color;

            Vec2Sub(&prevEndDir, &points[numPoints-1], &points[numPoints-2]);
            Vec2Nor2(&prevEndDir, &prevEndDir);
        }

        if (numCorners == 0)
        {
            for (int i = contourStart; i <= index; ++i)
            {
                glyph->flags[i] |= TT_WHITE;
            }
        }
        else if (
            // To avoid the teardrop case.
            numCorners > 1 &&
            Vec2Dot2(&prevEndDir, &firstStartDir) >= TT_CORNER_DOT_THRESH
        )
        {
            for (int i = edgeStart; i <= index; ++i)
            {
                // Reset color to magenta.
                glyph->flags[i] &= ~TT_WHITE;
                glyph->flags[i] |= TT_MAGENTA;
            }
        }

        index ++;
    }
}

// Signed-distance field.
// @TODO: Fix and optimize.
/*
extern void TT_RenderGlyph_SDF(STACKALLOC* alloc,
    unsigned char* dest, int destW, int destH,
    int offsetX, int offsetY, TT_GLYPHDATA* glyph,
    f32 scale, u32 padding, f32 distPixelRange)
{
    // Bounds checking on offset.
    if (offsetX < 0 || offsetY >= destW ||
        offsetY < 0 || offsetY >= destH) {
        return;
    }

    float xminScaled = (float) glyph->minX * scale;
    float xmaxScaled = (float) glyph->maxX * scale;

    // TTF uses +y up, bitmaps use +y down.
    float yminScaled = (float) glyph->minY * -scale;
    float ymaxScaled = (float) glyph->maxY * -scale;

    u32 width = (u32) ceilf(xmaxScaled - xminScaled) + padding * 2;
    u32 height = (u32) ceilf(ymaxScaled - yminScaled) + padding * 2;

    width = MIN(width, destW - (u32) offsetX);
    height = MIN(height, destH - (u32) offsetY);

    // Scratch arena.

    // TODO:

    //vec2* points = PUSH_ARRAY(glyph->nPoints);
    //vec2* points;
    vec2* points = (vec2 *) StackAlloc(alloc, glyph->nPoints * sizeof(vec2));

    for (int i = 0; i < glyph->nPoints; ++i) {
        points[i].x = (float) glyph->points[i].x * scale - xminScaled + (f32) padding;
        points[i].y = (float) glyph->points[i].y * -scale - yminScaled + (float) padding;
    }

    for (int yi = 0; yi < height; ++yi) {
        // Why height on both loops, though?
        for (int xi = 0; xi < width; ++xi) {
            vec2 p = { (float) xi + 0.5f, (float) yi + 0.5f };

            float minDist = INFINITY;

            int winding = TT_ComputeCrossingsX(p.x, p.y, points, glyph->nPoints, glyph);

            u32 index = 0;
            for (int seg = 0; seg < glyph->nSegments; ++seg) {
                float dist = INFINITY;

                if (glyph->flags[index] & TT_POINT_FLAG_LINE) {
                    // Find distance to line.
                    vec2 p0 = points[index++];
                    vec2 p1 = points[index];

                    vec2 lineVec; Vec2Sub(&lineVec, &p1, &p0);
                    vec2 pointVec; Vec2Sub(&pointVec, &p, &p0);

                    float t = Vec2Dot2(&pointVec, &lineVec) / Vec2Dot2(&lineVec, &lineVec);
                    t = CLAMP(t, 0.0f, 1.0f);

                    vec2 linePoint;
                    Vec2MulScalar(&lineVec, &lineVec, t);
                    Vec2Add(&linePoint, &p0, &lineVec);

                    dist = Vec2Dst2(&p, &linePoint);
                } else {
                    // Find distance to bezier curve.
                    vec2 p0 = points[index++];
                    vec2 p1 = points[index++];
                    vec2 p2 = points[index];

                    vec2 c0; Vec2Sub(&c0, &p, &p0);
                    vec2 c1; Vec2Sub(&c1, &p1, &p0);
                    vec2 tmp; Vec2MulScalar(&tmp, &p1, -2.0f);
                    Vec2Add(&tmp, &tmp, &p0);
                    vec2 c2; Vec2Add(&c2, &p2, &tmp);

                    float ts[3] = { 0 };
                    u32 num_t = SolveCubic(
                        ts,
                        Vec2Dot2(&c2, &c2),
                        3.0f * Vec2Dot2(&c1, &c2),
                        2.0f * Vec2Dot2(&c1, &c1) - Vec2Dot2(&c2, &c0),
                        -Vec2Dot2(&c1, &c0)
                    );

                    for (int i = 0; i < num_t; ++i) {
                        float t = CLAMP(ts[i], 0.0f, 1.0f);

                        vec2 sc2; Vec2MulScalar(&sc2, &c2, t*t);
                        vec2 sc1; Vec2MulScalar(&sc1, &c1, 2.0f * t);
                        vec2 bezPoint; Vec2Add(&bezPoint, &sc2, &sc1);
                        Vec2Add(&bezPoint, &bezPoint, &p0);

                        float currentDist = Vec2Dst2(&p, &bezPoint);
                        dist = MIN(dist, currentDist);
                    }
                }

                minDist = MIN(minDist, dist);

                if (glyph->flags[index] & TT_POINT_FLAG_CONTOUR_END) {
                    index ++;
                }
            }

            float scaledDist = CLAMP(minDist, -distPixelRange, distPixelRange);
            scaledDist *= 255.0f / (distPixelRange);

            if (winding == 0) {
                scaledDist = -scaledDist;  // if outside the shape, value is negative
            }

            float onedge_value = 0;
            float val = onedge_value + minDist * distPixelRange;
            if (val < 0)
               val = 0;
            else if (val > 255)
               val = 255;
            //data[(y-iy0)*w+(x-ix0)] = (unsigned char) val;


            u32 bmpIndex = ((xi + (u32) offsetX) + (yi + (u32) offsetY) * destW) * 4;
            unsigned char* pixel = dest + bmpIndex;

            // TODO: 1-component color?
            //u8 val = (u8) scaledDist;
            pixel[0] = val;
            pixel[1] = val;
            pixel[2] = val;
            pixel[3] = val;
        }
    }

    StackFree(alloc, points);
}
*/


static int GetGlyphBox(TT_FONTINFO *info, int glyph_index, int *x0, int *y0, int *x1, int *y1)
{
    //if (info->cff.size) {
    //   stbtt__GetGlyphInfoT2(info, glyph_index, x0, y0, x1, y1);
    //} else {
    //int g = stbtt__GetGlyfOffset(info, glyph_index);
    TT_GLYFENTRY ent = TT_FindGlyfEntry(info, glyph_index);
    if (ent.length < 10)
    {
        return 1;
    }

    int g = info->glyf.offset + ent.offset;
    //fprintf(stderr, "\n");
    fprintf(stderr, "%s: ok: infodata=%p g=%d\n", __func__, info->data.data, g);


    if (g < 0) return 1;

    if (x0) *x0 = (i16) TT_READ_BE16(info->data.data + g + 2);
    if (y0) *y0 = (i16) TT_READ_BE16(info->data.data + g + 4);
    if (x1) *x1 = (i16) TT_READ_BE16(info->data.data + g + 6);
    if (y1) *y1 = (i16) TT_READ_BE16(info->data.data + g + 8);
    fprintf(stderr, "%s: x0 y0 x1 y1: %d %d %d %d\n", __func__, *x0, *y0, *x1, *y1);


    return 0;
}



extern void TT_GetGlyphBitmapBoxSubpixel(TT_FONTINFO* info, int glyph,
    float scaleX, float scaleY, float shiftX, float shiftY, int* ix0, int* iy0, int* ix1, int* iy1)
{
    int x0=0,y0=0,x1,y1;
    if (GetGlyphBox(info, glyph, &x0,&y0,&x1,&y1))
    {
        // e.g. space character
        if (ix0) *ix0 = 0;
        if (iy0) *iy0 = 0;
        if (ix1) *ix1 = 0;
        if (iy1) *iy1 = 0;
    }
    else
    {
        if (ix0) *ix0 = floorf( x0 * scaleX + shiftX);
        if (iy0) *iy0 = floorf(-y1 * scaleY + shiftY);
        if (ix1) *ix1 = ceilf( x1 * scaleX + shiftX);
        if (iy1) *iy1 = ceilf(-y0 * scaleY + shiftY);
    }
}



extern unsigned char* TT_RenderGlyph_SDF(
    TT_FONTINFO* info, float scale, int glyphIndex,
    int padding, unsigned char onEdgeValue, float pixelDistScale,
    int* width, int* height, int* xoff, int* yoff)
{
    float scale_x = scale, scale_y = scale;
    int ix0,iy0,ix1,iy1;
    int w,h;
    unsigned char *data;

    if (scale == 0) {
        fprintf(stderr, "%s: ok.\n", __func__);
        return NULL;
    }

    TT_GetGlyphBitmapBoxSubpixel(info, glyphIndex, scale, scale, 0.0f, 0.0f, &ix0,&iy0,&ix1,&iy1);

   // if empty, return NULL
   if (ix0 == ix1 || iy0 == iy1)
   {
       fprintf(stderr, "%s: skill issue.\n", __func__);
      return NULL;
   }

   ix0 -= padding;
   iy0 -= padding;
   ix1 += padding;
   iy1 += padding;

   w = (ix1 - ix0);
   h = (iy1 - iy0);
 
   if (width ) *width  = w;
   if (height) *height = h;
   if (xoff  ) *xoff   = ix0;
   if (yoff  ) *yoff   = iy0;

   // invert for y-downwards bitmaps
   scale_y = -scale_y;

   // TODO: Issue here.
   TT_GLYPHDATA glyph = TT_GlyphDataFromIndex(info, glyphIndex);
   int x, y, i, j;
   float* precompute;
   vec2* verts;

   {
        // distance from singular values (in the same units as the pixel grid)
        const float eps = 1./1024, eps2 = eps*eps;
        const int numVerts = glyph.nPoints;

        // Init RGBA color image.
        {
            size_t bytecount = w * h * sizeof(unsigned char) * 4;
            data = (unsigned char *) _MALLOC(bytecount);
            if (data == NULL)
            {
                fprintf(stderr, "%s: Failed to allocate image data!\n", __func__);
                return NULL;
            }
            _MEMSET(data, 0, bytecount);
        }

        verts = (vec2 *) _MALLOC(glyph.nPoints * sizeof(vec2));

        size_t bytecount = glyph.nSegments * sizeof(float);
        precompute = (float *) _MALLOC(bytecount);
        _MEMSET(precompute, 0, bytecount);

        for (int i = 0; i < glyph.nPoints; ++i)
        {
            verts[i].x = (float) glyph.points[i].x * scale_x;
            verts[i].y = (float) glyph.points[i].y * scale_y;
        }

        u32 index = 0;
        for (int seg = 0; seg < glyph.nSegments; ++seg)
        {
            if (glyph.flags[index] & TT_POINT_FLAG_LINE)
            {
                // Find distance to line.
                vec2i16 p0 = glyph.points[index++];
                vec2i16 p1 = glyph.points[index];

                float x0 = p0.x*scale_x, y0 = p0.y*scale_y;
                float x1 = p1.x*scale_x, y1 = p1.y*scale_y;
                float dist = (float) sqrtf((x1-x0)*(x1-x0) + (y1-y0)*(y1-y0));
                precompute[seg] = (dist < eps) ? 0.0f : 1.0f / dist;
            }
            else
            {
                // Bezier curve.
                vec2i16 p0 = glyph.points[index++];
                vec2i16 p1 = glyph.points[index++];
                vec2i16 p2 = glyph.points[index];

                float x2 = p2.x*scale_x, y2 = p2.y*scale_y;
                float x1 = p1.x*scale_x, y1 = p1.y*scale_y;
                float x0 = p0.x*scale_x, y0 = p0.y*scale_y;
                float bx = x0 - 2*x1 + x2, by = y0 - 2*y1 + y2;
                float len2 = bx*bx + by*by;

                if (len2 >= eps2) {
                    precompute[seg] = 1.0f / len2;
                } else {
                    precompute[seg] = 0.0f;
                }
            }
        }

    //fprintf(stderr, "%s: glyph segments: %d\n", __func__, glyph->nSegments);

    for (y=iy0; y < iy1; ++y)
    {
        for (x=ix0; x < ix1; ++x)
        {
            float val;
            float min_dist = 999999.0f;
            float sx = (float) x + 0.5f;
            float sy = (float) y + 0.5f;
            float x_gspace = (sx / scale_x);
            float y_gspace = (sy / scale_y);

            int winding = TT_ComputeCrossingsX(x_gspace, y_gspace, verts, glyph.nPoints, &glyph); // @OPTIMIZE: this could just be a rasterization, but needs to be line vs. non-tesselated curves so a new path

            int index = 0;
            for (i=0; i < glyph.nSegments; ++i)
            {
                vec2i16 p0 = glyph.points[index++];
                //float x0 = .x*scale_x, y0 = verts[i].y*scale_y;
                float x0 = p0.x, y0 = p0.y;

                if ((glyph.flags[i] & TT_POINT_FLAG_LINE) && precompute[i] != 0.0f)
                {
                    vec2i16 p1 = glyph.points[index++];
                    //float x1 = verts[i-1].x*scale_x, y1 = verts[i-1].y*scale_y;
                    float x1 = p1.x, y1 = p1.y;

                    float dist,dist2 = (x0-sx)*(x0-sx) + (y0-sy)*(y0-sy);
                    if (dist2 < min_dist*min_dist) {
                        min_dist = (float) sqrt(dist2);
                    }

                    // coarse culling against bbox
                    //if (sx > MIN(x0,x1)-min_dist && sx < MAX(x0,x1)+min_dist &&
                    //    sy > MIN(y0,y1)-min_dist && sy < MAX(y0,y1)+min_dist)
                    dist = (float) fabs((x1-x0)*(y0-sy) - (y1-y0)*(x0-sx)) * precompute[i];
                    assert(i != 0);
                    if (dist < min_dist)
                    {
                        // check position along line
                        // x' = x0 + t*(x1-x0), y' = y0 + t*(y1-y0)
                        // minimize (x'-sx)*(x'-sx)+(y'-sy)*(y'-sy)
                        float dx = x1-x0, dy = y1-y0;
                        float px = x0-sx, py = y0-sy;
                        // minimize (px+t*dx)^2 + (py+t*dy)^2 = px*px + 2*px*dx*t + t^2*dx*dx + py*py + 2*py*dy*t + t^2*dy*dy
                        // derivative: 2*px*dx + 2*py*dy + (2*dx*dx+2*dy*dy)*t, set to 0 and solve
                        float t = -(px*dx + py*dy) / (dx*dx + dy*dy);
                        if (t >= 0.0f && t <= 1.0f) {
                            min_dist = dist;
                        }
                    }
               }
                else
               {
                    vec2i16 p1 = glyph.points[index++];
                    vec2i16 p2 = glyph.points[index];

                    float x2 = p2.x, y2 = p2.y;
                    float x1 = p1.x, y1 = p1.y;

                    float box_x0 = MIN(MIN(x0,x1),x2);
                    float box_y0 = MIN(MIN(y0,y1),y2);
                    float box_x1 = MAX(MAX(x0,x1),x2);
                    float box_y1 = MAX(MAX(y0,y1),y2);
                    // coarse culling against bbox to avoid computing cubic unnecessarily
                    if (sx > box_x0-min_dist && sx < box_x1+min_dist && sy > box_y0-min_dist && sy < box_y1+min_dist)
                    {
                        int num=0;
                        float ax = x1-x0, ay = y1-y0;
                        float bx = x0 - 2*x1 + x2, by = y0 - 2*y1 + y2;
                        float mx = x0 - sx, my = y0 - sy;
                        float res[3] = {0.f,0.f,0.f};
                        float px,py,t,it,dist2;
                        float a_inv = precompute[i];
                        if (a_inv == 0.0)
                        { // if a_inv is 0, it's 2nd degree so use quadratic formula
                            float a = 3*(ax*bx + ay*by);
                            float b = 2*(ax*ax + ay*ay) + (mx*bx+my*by);
                            float c = mx*ax+my*ay;
                            if (fabs(a) < eps2)
                            { // if a is 0, it's linear
                                if (fabs(b) >= eps2) {
                                    res[num++] = -c/b;
                                }
                            }
                            else
                            {
                                float discriminant = b*b - 4*a*c;
                                if (discriminant < 0) {
                                    num = 0;
                                } else {
                                    float root = (float) sqrtf(discriminant);
                                    res[0] = (-b - root)/(2*a);
                                    res[1] = (-b + root)/(2*a);
                                    num = 2; // don't bother distinguishing 1-solution case, as code below will still work
                                }
                            }
                        }
                        else
                        {
                            float b = 3*(ax*bx + ay*by) * a_inv; // could precompute this as it doesn't depend on sample point
                            float c = (2*(ax*ax + ay*ay) + (mx*bx+my*by)) * a_inv;
                            float d = (mx*ax+my*ay) * a_inv;
                            num = SolveCubicNormed(res, b, c, d);
                        }

                        dist2 = (x0-sx)*(x0-sx) + (y0-sy)*(y0-sy);
                        if (dist2 < min_dist*min_dist)
                        {
                            min_dist = (float) sqrtf(dist2);
                        }


                        if (num >= 1 && res[0] >= 0.0f && res[0] <= 1.0f)
                        {
                            t = res[0], it = 1.0f - t;
                            px = it*it*x0 + 2*t*it*x1 + t*t*x2;
                            py = it*it*y0 + 2*t*it*y1 + t*t*y2;
                            dist2 = (px-sx)*(px-sx) + (py-sy)*(py-sy);
                            if (dist2 < min_dist * min_dist) {
                                min_dist = (float) sqrtf(dist2);
                            }
                        }

                        if (num >= 2 && res[1] >= 0.0f && res[1] <= 1.0f)
                        {
                            t = res[1], it = 1.0f - t;
                            px = it*it*x0 + 2*t*it*x1 + t*t*x2;
                            py = it*it*y0 + 2*t*it*y1 + t*t*y2;
                            dist2 = (px-sx)*(px-sx) + (py-sy)*(py-sy);
                            if (dist2 < min_dist * min_dist) {
                                min_dist = (float) sqrtf(dist2);
                            }
                        }

                        if (num >= 3 && res[2] >= 0.0f && res[2] <= 1.0f)
                        {
                            t = res[2], it = 1.0f - t;
                            px = it*it*x0 + 2*t*it*x1 + t*t*x2;
                            py = it*it*y0 + 2*t*it*y1 + t*t*y2;
                            dist2 = (px-sx)*(px-sx) + (py-sy)*(py-sy);
                            if (dist2 < min_dist * min_dist) {
                                min_dist = (float) sqrtf(dist2);
                            }
                        }
                    }
                }
            }
            if (winding == 0)
            {
               min_dist = -min_dist;  // if outside the shape, value is negative
            }
            val = onEdgeValue + pixelDistScale * min_dist;
            if (val < 0) {
               val = 0;
            } else if (val > 255) {
               val = 255;
            }

            u32 bitmapIndex = ((y - iy0)*w + (x - ix0)) * 4;
            unsigned char* pixel = data + bitmapIndex;

            unsigned char color = (unsigned char) val;
            pixel[0] = color;
            pixel[1] = color;
            pixel[2] = color;
            pixel[3] = color;
         }
      }

    _FREE(precompute);
    _FREE(verts);
    }

    fprintf(stderr, "%s: ok: (data, w, h): (%p, %d, %d) \n", __func__, data, w, h);


    return data;
}


extern void TT_RenderGlyph_Scanline(STACKALLOC* alloc,
    unsigned char* dest, int destW, int destH,
    float scale, float padding)
{

}



static int TT_RayIntersectBezier(float orig[2], float ray[2], float q0[2], float q1[2], float q2[2], float hits[2][2])
{
    float q0perp = q0[1]*ray[0] - q0[0]*ray[1];
    float q1perp = q1[1]*ray[0] - q1[0]*ray[1];
    float q2perp = q2[1]*ray[0] - q2[0]*ray[1];
    float roperp = orig[1]*ray[0] - orig[0]*ray[1];

    float a = q0perp - 2*q1perp + q2perp;
    float b = q1perp - q0perp;
    float c = q0perp - roperp;

    float s0 = 0., s1 = 0.;
    int num_s = 0;

    if (a != 0.0)
    {
        float discr = b*b - a*c;
        if (discr > 0.0) {
            float rcpna = -1 / a;
            float d = (float) sqrtf(discr);
            s0 = (b+d) * rcpna;
            s1 = (b-d) * rcpna;

            if (s0 >= 0.0 && s0 <= 1.0) {
                num_s = 1;
            }
            if (d > 0.0 && s1 >= 0.0 && s1 <= 1.0) {
                if (num_s == 0) s0 = s1;
                ++num_s;
            }
        }
    }
    else
    {
        // 2*b*s + c = 0
        // s = -c / (2*b)
        s0 = c / (-2 * b);
        if (s0 >= 0.0 && s0 <= 1.0) {
            num_s = 1;
        }
   }

   if (num_s == 0)
   {
        return 0;
   }
   else
   {
        float rcp_len2 = 1 / (ray[0]*ray[0] + ray[1]*ray[1]);
        float rayn_x = ray[0] * rcp_len2, rayn_y = ray[1] * rcp_len2;

        float q0d =   q0[0]*rayn_x +   q0[1]*rayn_y;
        float q1d =   q1[0]*rayn_x +   q1[1]*rayn_y;
        float q2d =   q2[0]*rayn_x +   q2[1]*rayn_y;
        float rod = orig[0]*rayn_x + orig[1]*rayn_y;

        float q10d = q1d - q0d;
        float q20d = q2d - q0d;
        float q0rd = q0d - rod;

        hits[0][0] = q0rd + s0*(2.0f - 2.0f*s0)*q10d + s0*s0*q20d;
        hits[0][1] = a*s0+b;

        if (num_s > 1)
        {
            hits[1][0] = q0rd + s1*(2.0f - 2.0f*s1)*q10d + s1*s1*q20d;
            hits[1][1] = a*s1+b;
            return 2;
        }
        else
        {
            return 1;
        }
    }
}

// Get the amount of intersections in the X axis.
// Took from stb_truetype.h
extern int TT_ComputeCrossingsX(float x, float y, vec2* verts, int nverts, TT_GLYPHDATA* const glyph)
{
    int i;
    float orig[2], ray[2] = { 1, 0 };
    float y_frac;
    int winding = 0;

    // make sure y never passes through a vertex of the shape
    y_frac = (float) fmod(y, 1.0f);
    if (y_frac < 0.01f)
    {
        y += 0.01f;
    }
    else if (y_frac > 0.99f)
    {
        y -= 0.01f;
    }
    orig[0] = x;
    orig[1] = y;


    // test a ray from (-infinity,y) to (x,y)
    u32 index = 0;
    for (int seg = 0; seg < glyph->nSegments; ++seg)
    {
        if (glyph->flags[index] & TT_POINT_FLAG_LINE)
        {
            vec2 p0 = verts[index++];
            vec2 p1 = verts[index];

            if (y > MIN(p0.y,p1.y) && y < MAX(p0.y,p1.y) && x > MIN(p0.x,p1.x))
            {
               float x_inter = (y - p0.y) / (p1.y - p0.y) * (p1.x-p0.x) + p0.x;
               if (x_inter < x)
                    winding += (p0.y < p1.y) ? 1 : -1;
            }
        }
        else
        {
            // It's a Bezier curve.
            int i = index;
            vec2 p0 = verts[index++];
            vec2 p1 = verts[index++];
            vec2 p2 = verts[index];

            int ax = MIN(p0.x,MIN(p1.x,p2.x)), ay = MIN(p0.y,MIN(p1.y,p2.y));
            int by = MAX(p0.y,MAX(p1.y,p2.y));
            if (y > ay && y < by && x > ax)
            {
                float q0[2],q1[2],q2[2];
                float hits[2][2];
                q0[0] = (float)p0.x;
                q0[1] = (float)p0.y;
                q1[0] = (float)p1.x;
                q1[1] = (float)p1.y;
                q2[0] = (float)p2.x;
                q2[1] = (float)p2.y;
                if (Equal(q0,q1) || Equal(q1,q2))
                {
                    p0.x = (int)verts[i+1].x;
                    p0.y = (int)verts[i+1].y;
                    p1.x = (int)verts[i  ].x;
                    p1.y = (int)verts[i  ].y;
                    if (y > MIN(p0.y,p1.y) && y < MAX(p0.y,p1.y) && x > MIN(p0.x,p1.x))
                    {
                        float x_inter = (y - p0.y) / (p1.y - p0.y) * (p1.x-p0.x) + p0.x;
                        if (x_inter < x)
                            winding += (p0.y < p1.y) ? 1 : -1;
                    }
                 }
                else
                {
                    int num_hits = TT_RayIntersectBezier(orig, ray, q0, q1, q2, hits);
                    if (num_hits >= 1)
                    {
                        if (hits[0][0] < 0)
                            winding += (hits[0][1] < 0 ? -1 : 1);
                        if (num_hits >= 2)
                            if (hits[1][0] < 0)
                                winding += (hits[1][1] < 0 ? -1 : 1);
                    }
                }
            }
        }
   }

   return winding;
}
