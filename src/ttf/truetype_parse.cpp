#include "truetype_parse.h"
#include "truetype.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//#include "../misc/mem_heap.h"



u32 TT_CalcChecksum(STRING8 buf, u32 offset, u32 len);
// Assumes loca is already validated.
//extern b32 TT_ValidateLoca(STRING8 buf, const TT_FONTINFO* info);
// Assumes cmap is already validated.
b32 TT_FindCmapSubtable(STRING8 buf, TT_FONTINFO* info, TT_FONTTABLE cmap);

TT_GLYFENTRY TT_FindGlyfEntry(TT_FONTINFO* info, u32 glyphIndex);



extern void TT_FontInit(STRING8 buf, TT_FONTINFO *info)
{
    fprintf(stderr, "%s: Buf: data:%p, len:%d\n", __func__, buf.data, (int) buf.len);


    if (buf.len <= 12)
    {
        fprintf(stderr, "%s: Cannot parse TTF (invalid buffer).\n", __func__);
        info->initialized = FALSE;
    }

    u32 scalarType = TT_READ_BE32(buf.data);
    if (scalarType != 0x00010000 && scalarType != 0x74727565)
    {
        fprintf(stderr, "%s: Cannot parse TTF (invalid file type).\n", __func__);
        info->initialized = FALSE;
    }

    u32 fileChecksum = TT_CalcChecksum(buf, 0, (u32) buf.len);
    if (fileChecksum != 0xB1B0AFBA)
    {
        fprintf(stderr, "%s: Invalid/corrupted TTF file (mismatched checksum)!\n", __func__);
        info->initialized = FALSE;
    }

    u16 numTables = TT_READ_BE16(buf.data + 4);
    if (buf.len <= 12 + numTables * 16)
    {
        fprintf(stderr, "%s: Cannot parse TTF. It's too large.\n", __func__);
        info->initialized = FALSE;
    }

    TT_FONTTABLE cmap{0};
    TT_FONTTABLE maxp{0};

    // Checks the integrity of the tables.
    if (!TT_GetValidateTable(buf, TT_TAG("head"), &info->head) ||
        !TT_GetValidateTable(buf, TT_TAG("glyf"), &info->glyf) ||
        !TT_GetValidateTable(buf, TT_TAG("hmtx"), &info->hmtx) ||
        !TT_GetValidateTable(buf, TT_TAG("loca"), &info->loca) ||
        !TT_GetValidateTable(buf, TT_TAG("cmap"), &cmap) ||
        !TT_GetValidateTable(buf, TT_TAG("maxp"), &maxp) ||
        (info->head.length != 54 || maxp.length < 6))
    {
        fprintf(stderr, "%s: Cannot parse TTF (invalid tables).\n", __func__);
        goto invalid;
    }

    info->loca_format = (i16) TT_READ_BE16(buf.data + info->head.offset + 50);
    if (info->loca_format != 0 && info->loca_format != 1)
    {
        fprintf(stderr, "%s: Cannot parse TTF (invalid loca format).\n", __func__);
        goto invalid;
    }
    if (!TT_ValidateLoca(buf, info))
    {
        fprintf(stderr, "%s: Cannot parse TTF (invalid loca offsets).\n", __func__);
        goto invalid;
    }

    // Maxp parsing.
    {
        if (TT_READ_BE32(buf.data + maxp.offset) != 0x00010000 ||  maxp.length != 32)
        {
            fprintf(stderr, "%s: Couldn't parse TTF (CFF glyphs not supported).\n", __func__);
            goto invalid;
        }

        info->nGlyphs = TT_READ_BE16(buf.data + maxp.offset + 4);
        u32 loca_size = info->loca.length / (info->loca_format == 0 ? 2 : 4);
        if (loca_size < info->nGlyphs + 1)
        {
            fprintf(stderr, "%s: Cannot parse TTF (invalid loca size).\n", __func__);
            goto invalid;
        }


        u16 maxPoints = TT_READ_BE16(buf.data + maxp.offset + 6);
        u16 maxCompositePoints = TT_READ_BE16(buf.data + maxp.offset + 10);
        u16 maxContours = TT_READ_BE16(buf.data + maxp.offset + 8);
        u16 maxCompositeContours = TT_READ_BE16(buf.data + maxp.offset + 8); // Same value.

        info->maxGlyphContours = (u32) MAX(maxContours, maxCompositeContours);

        // The processing of glyphs involves generating implied control points
        // and duplicating the first point to close the contour.
        // These calculations are for that.
        info->maxGlyphPoints = (u32)
            MAX(maxPoints, maxCompositePoints) * 2 + info->maxGlyphContours;
    }

    if (!TT_FindCmapSubtable(buf, info, cmap))
    {
        fprintf(stderr, "%s: Cannot parse TTF (unable to find supported cmap).\n", __func__);
        goto invalid;
    }

    info->data = buf;
    info->initialized = TRUE;

    return;

invalid:
    info->initialized = FALSE;
}

float TT_ScaleForEm(STRING8 buf, TT_FONTINFO *info, float pixelsPerEm)
{
    if (info == NULL || !info->initialized)
    {
        return 1.0f;
    }
    float upm = (float) TT_READ_BE16(buf.data + info->head.offset + 18);
    return pixelsPerEm / upm;
}


// Used for loading in glyphs before the number of total points is known.
typedef struct TEMP_GLYPH {
    u32 numContours;
    u32 numSegments;
    u32 numPoints;

    u32 maxPoints;

    TT_POINTFLAG* flags;
    vec2i16* points;
} TEMP_GLYPH;


// Recursively adds points to the "temporary glyph" struct.
static b32 TT_GlyphAddPoints(TT_FONTINFO* info,
    TEMP_GLYPH* glyph, u32 glyphIndex)
{
    TT_GLYFENTRY entry = TT_FindGlyfEntry(info, glyphIndex);
    if (entry.length < 10)
    {
        return FALSE;
    }

    u8* glyfData = info->data.data + info->glyf.offset + entry.offset;
    i16 numContours = (i16) TT_READ_BE16(glyfData);

    if (numContours > 0)
    {
        // Simple glyph description.
        if (entry.length < 12 + (u32) numContours * 2)
        {
            return FALSE;
        }

        u16 instructionLength = TT_READ_BE16(glyfData + 10 + numContours * 2);
        if (entry.length < 12 + (u32) numContours * 2 + instructionLength)
        {
            return FALSE;
        }

        glyph->numContours += (u32) numContours;


        u32 numRawPoints = (u32) TT_READ_BE16(glyfData + 10 + (numContours - 1) * 2) + 1;

        // TODO: Allocate arena.
        u8* flagsRaw = (u8 *) _MALLOC(numRawPoints * sizeof(u8));
        vec2i16* pointsRaw = (vec2i16 *) _MALLOC(numRawPoints * sizeof(vec2i16));

        u32 dataOffset = 12 + (u32) numContours * 2 + instructionLength;
        u32 numFlags = 0;

        while (numFlags < numRawPoints && dataOffset < entry.length)
        {
            u8 flag = *(glyfData + dataOffset);
            dataOffset ++;

            flagsRaw[numFlags++] = flag;

            // Checking REPEAT flag.
            if ((flag & 0x08) && dataOffset < numRawPoints)
            {
                u8 count = *(glyfData + dataOffset);
                dataOffset ++;

                while (count-- && numFlags < numRawPoints)
                {
                    flagsRaw[numFlags] = flag;
                }
            }
        }

        i16 x = 0;
        for (int i = 0; i < numRawPoints; ++i)
        {
            if ((flagsRaw[i] & 0x02) && dataOffset < entry.length)
            {
                // 1 byte x;
                u8 diff = *(glyfData + dataOffset);
                dataOffset ++;

                // Checking if diff is positive.
                x += ((flagsRaw[i] & 0x10) ? (i16) diff : -(i16) diff);
            }
            else if ((flagsRaw[i] & 0x10) != 0x10 && dataOffset + 1 < entry.length)
            {
                // If x is not the same (0x10), then get the new diff.
                i16 diff = (i16) TT_READ_BE16(glyfData + dataOffset);
                dataOffset += 2;

                x += diff;
            }

            pointsRaw[i].x = x;
        }

        // Do the same for y.
        i16 y = 0;
        for (int i = 0; i < numRawPoints; ++i)
        {
            if ((flagsRaw[i] & 0x04) && dataOffset < entry.length)
            {
                // 1 byte x;
                u8 diff = *(glyfData + dataOffset);
                dataOffset ++;

                // Checking if diff is positive.
                y += ((flagsRaw[i] & 0x20) ? (i16) diff : -(i16) diff);
            }
            else if ((flagsRaw[i] & 0x20) != 0x20 && dataOffset + 1 < entry.length)
            {
                // If x is not the same (0x20), then get the new diff.
                i16 diff = (i16) TT_READ_BE16(glyfData + dataOffset);
                dataOffset += 2;

                y += diff;
            }

            pointsRaw[i].y = y;
        }

        // Contours.
        for (int c = 0; c < numContours; ++c)
        {
            int startPoint = c <= 0 ? 0 : (TT_READ_BE16(glyfData + 10 + (c-1) * 2) + 1);
            int endPoint = TT_READ_BE16(glyfData + 10 + c*2);

            int numPoints = endPoint = startPoint + 1;
            int pointOffset = 0;
            b8 justOffset = FALSE;

            for (int i = 0; i < (i32) numPoints; ++i)
            {
                u32 p0i = (((u32) (i + pointOffset + 0) % numPoints) + startPoint);
                u32 p1i = (((u32) (i + pointOffset + 1) % numPoints) + startPoint);
                u32 p2i = (((u32) (i + pointOffset + 2) % numPoints) + startPoint);

                vec2i16 p0 = pointsRaw[p0i];
                vec2i16 p1 = pointsRaw[p1i];
                vec2i16 p2 = pointsRaw[p2i];

                TT_POINTFLAG p0flag = justOffset ? TT_POINT_FLAG_CONTOUR_OFFSET : 0;
                TT_POINTFLAG p1flag = 0;
                TT_POINTFLAG p2flag = 0;

                justOffset = FALSE;

                b8 bez = TRUE, skip = FALSE;

                u32 onCurveBits = (u32) (
                    (flagsRaw[p0i] & 0x1) << 2 |
                    (flagsRaw[p1i] & 0x1) << 1 |
                    (flagsRaw[p2i] & 0x1)
                );

                switch (onCurveBits)
                {
                    case 0b110:
                    case 0b111:
                    {
                        // Line segment.
                        bez = false;
                        p2 = p1;
                        p0flag |= TT_POINT_FLAG_LINE;
                    } break;

                    case 0b101:
                    {
                        // Bezier (all points explicit).
                        i++;
                    } break;

                    case 0b100:
                    {
                        // Bezier (end point implicit).
                        p2 = CLITERAL(vec2i16) {
                            (i16) (p1.x + p2.x),
                            (i16) (p1.y + p2.y),
                        };

                        p2flag |= TT_POINT_FLAG_GENERATED;
                    } break;

                    case 0b001:
                    {
                        // Bezier (start point implicit).
                        p0 = CLITERAL(vec2i16) {
                            (i16)((p0.x + p1.x) / 2),
                            (i16)((p0.y + p1.y) / 2),
                        };

                        p0flag |= TT_POINT_FLAG_GENERATED;

                        // Skip off curve point.
                        i++;
                    } break;

                    case 0b000:
                    {
                        p0 = CLITERAL(vec2i16) {
                            (i16)((p0.x + p1.x) / 2),
                            (i16)((p0.y + p1.y) / 2),
                        };
                        p2 = CLITERAL(vec2i16) {
                            (i16)((p1.x + p2.x) / 2),
                            (i16)((p1.y + p2.y) / 2),
                        };

                        p0flag |= TT_POINT_FLAG_GENERATED;
                        p2flag |= TT_POINT_FLAG_GENERATED;
                    } break;

                    case 0b010:
                    case 0b011:
                    {
                        // Undefined case, adjust offset until it makes sense.
                        pointOffset ++;
                        i --;

                        justOffset = TRUE;
                        skip = TRUE;
                    } break;
                }

                if (skip)
                {
                    continue;
                }

                glyph->numSegments ++;
                glyph->numPoints ++;
                glyph->flags[glyph->numPoints-1] = p0flag;
                glyph->points[glyph->numPoints-1] = p0;

                if (bez)
                {
                    glyph->numPoints ++;
                    glyph->flags[glyph->numPoints-1] = p1flag;
                    glyph->points[glyph->numPoints-1] = p1;
                }

                if (i == (i32) numPoints - 1)
                {
                    p2flag |= TT_POINT_FLAG_CONTOUR_END;

                    glyph->numPoints ++;
                    glyph->flags[glyph->numPoints-1] = p2flag;
                    glyph->points[glyph->numPoints-1] = p2;
                }
            }
        }

        // Release scratch arena.
        // TODO: Maybe this will fail.
        _FREE(flagsRaw);
        _FREE(pointsRaw);
    }
    else if (numContours < 0)
    {
        // Compound glyph description.

        u32 dataOffset = 10;
        b8 more = TRUE;
        while (more && dataOffset < entry.length)
        {
            if (dataOffset + 4 > entry.length)
            {
                return FALSE;
            }

            u16 flags = TT_READ_BE16(glyfData + dataOffset + 0);
            u16 childGlyphIndex = TT_READ_BE16(glyfData + dataOffset + 2);
            dataOffset += 4;

            i16 rawxoffset = 0, rawyoffset = 0;
            i32 parentPoint = -1, childPoint = -1;

            b8 scaleOffset = FALSE;

            if (flags & 0x0002)
            {
                // ARGS_ARE_XY_VALUES.
                if (flags & 0x0001 && dataOffset + 4 <= entry.length)
                {
                    // 16-bit offsets.
                    rawxoffset = (i16) TT_READ_BE16(glyfData + dataOffset + 0);
                    rawyoffset = (i16) TT_READ_BE16(glyfData + dataOffset + 2);

                    dataOffset += 4;
                }
                else if (dataOffset + 2 <= entry.length)
                {
                    // 8-bit offsets.
                    rawxoffset = (i8) (*(glyfData + dataOffset + 0));
                    rawyoffset = (i8) (*(glyfData + dataOffset + 2));

                    dataOffset += 2;
                }

                if (flags & 0x0800)
                {
                    // SCALED_COMPONENT_OFFSET.
                    scaleOffset = TRUE;
                }

                // This is defined as the default behaviour,
                // so that's why if instead of else if.
                if (flags & 0x1000)
                {
                    // UNSCALED_COMPONENT_OFFSET.
                    scaleOffset = FALSE;
                }
            }
            else
            {
                // Not ARGS_ARE_XY_VALUES.
                // (args are point numbers).
                if (flags & 0x0001 && dataOffset + 4 <= entry.length)
                {
                    // 16-bit point indices.
                    parentPoint = TT_READ_BE16(glyfData + dataOffset + 0);
                    childPoint = TT_READ_BE16(glyfData + dataOffset + 2);

                    dataOffset += 4;
                }
                else if (dataOffset + 2 <= entry.length)
                {
                    // 8-bit point indices.
                    parentPoint = *(glyfData + dataOffset + 0);
                    childPoint = *(glyfData + dataOffset + 1);

                    dataOffset += 2;
                }
            }

            // Row-major.
            float mat[4] = {
                1.0f, 0.0f,
                0.0f, 1.0f,
            };

            if ((flags & 0x0008) && dataOffset + 2 <= entry.length)
            {
                // WE_HAVE_A_SCALE.
                i16 scale2_14 = (i16) TT_READ_BE16(glyfData + dataOffset);
                dataOffset += 2;

                f32 scale = (f32) scale2_14 / (float)(1 << 14);
                mat[0] = scale;
                mat[3] = scale;
            }
            else if ((flags & 0x0040) && dataOffset + 4 <= entry.length)
            {
                // WE_HAVE_AN_X_AND_Y_SCALE.
                i16 xscale2_14 = (i16) TT_READ_BE16(glyfData + dataOffset + 0);
                i16 yscale2_14 = (i16) TT_READ_BE16(glyfData + dataOffset + 2);
                dataOffset += 4;

                mat[0] = (float) xscale2_14 / (float) (1 << 14);
                mat[3] = (float) yscale2_14 / (float) (1 << 14);
            } 
            else if ((flags & 0x0080) && dataOffset + 8 <= entry.length)
            {
                // WE_HAVE_A_2X2.
                i16 m00_2_14 = (i16) TT_READ_BE16(glyfData + dataOffset + 0);
                i16 m10_2_14 = (i16) TT_READ_BE16(glyfData + dataOffset + 2);
                i16 m01_2_14 = (i16) TT_READ_BE16(glyfData + dataOffset + 4);
                i16 m11_2_14 = (i16) TT_READ_BE16(glyfData + dataOffset + 6);
                dataOffset += 8;

                mat[0] = (float) m00_2_14 / (float) (1 << 14);
                mat[1] = (float) m01_2_14 / (float) (1 << 14);
                mat[2] = (float) m10_2_14 / (float) (1 << 14);
                mat[3] = (float) m11_2_14 / (float) (1 << 14);
            }

            u32 childStartIndex = glyph->numPoints;
            if (!TT_GlyphAddPoints(info, glyph, childGlyphIndex))
            {
                return FALSE;
            }

            if (parentPoint >= 0 && childPoint >= 0)
            {
                fprintf(stderr, "%s: WARN: TTF compound point aligning unsupported.\n", __func__);
            }

            f32 xoffset = (float) rawxoffset;
            f32 yoffset = (float) rawyoffset;
            if (scaleOffset)
            {
                xoffset = mat[0] * (float) rawxoffset + mat[1] * (float) rawyoffset;
                yoffset = mat[2] * (float) rawxoffset + mat[3] * (float) rawyoffset;
            }

            for (int i = childStartIndex; i < glyph->numPoints; ++i)
            {
                float x = (float) glyph->points[i].x;
                float y = (float) glyph->points[i].y;

                glyph->points[i].x = (i16) (mat[0] * x + mat[1] * y + xoffset);
                glyph->points[i].y = (i16) (mat[2] * x + mat[3] * y + yoffset);
            }


            more = (flags & 0x0020) == 0x0020;
        }
    }

    return TRUE;
}


TT_GLYPHDATA TT_GlyphDataFromIndex(TT_FONTINFO *info, u32 glyphIndex)
{
    if (info == NULL || !info->initialized)
    {
        return CLITERAL(TT_GLYPHDATA) { 0 };
    }

    TT_GLYFENTRY entry = TT_FindGlyfEntry(info, glyphIndex);
    if (entry.length < 10)
    {
        return CLITERAL(TT_GLYPHDATA) { 0 };
    }


    u32 offset = info->glyf.offset + entry.offset;

    TT_GLYPHDATA glyph = { 0 };
    glyph.minX = (i16) TT_READ_BE16(info->data.data + offset + 2);
    glyph.minY = (i16) TT_READ_BE16(info->data.data + offset + 4);
    glyph.maxX = (i16) TT_READ_BE16(info->data.data + offset + 6);
    glyph.maxY = (i16) TT_READ_BE16(info->data.data + offset + 8);

    fprintf(stderr, "%s: x0 y0 x1 y1: %d %d %d %d\n", __func__, glyph.minX, glyph.minY, glyph.maxX, glyph.maxY);


    // Here, I add in all the implied bezier points and duplicate the contour
    // start/end point. As such, this should be a sufficient upper bound.
    u32 maxPoints = info->maxGlyphPoints * 2 + info->maxGlyphContours;

    TEMP_GLYPH tempGlyph = { 0 };
    tempGlyph.maxPoints = maxPoints;
    tempGlyph.flags = (TT_POINTFLAG *) _MALLOC(maxPoints * sizeof(TT_POINTFLAG));
    tempGlyph.points = (vec2i16 *) _MALLOC(maxPoints * sizeof(vec2i16));

    //fprintf(stderr, "%s: flags=%p points=%p\n", __func__, tempGlyph.flags, tempGlyph.points);

    // TODO: Fix issue.
    if (!TT_GlyphAddPoints(info, &tempGlyph, glyphIndex))
    {
        fprintf(stderr, "%s: Failed to parse TTF glyph.\n", __func__);
        //ArenaScratchRelease(scratch);
        //StackFree(alloc, tempGlyph.flags);
        //StackFree(alloc, tempGlyph.points);

        return CLITERAL(TT_GLYPHDATA) { 0 };
    }

    glyph.nConturs = tempGlyph.numContours;
    glyph.nSegments = tempGlyph.numSegments;
    glyph.nPoints = tempGlyph.numPoints;

    //glyph.flags =;
    //glyph.points = ;
    glyph.flags = (TT_POINTFLAG *) _MALLOC(glyph.nPoints * sizeof(TT_POINTFLAG));
    glyph.points = (vec2i16 *) _MALLOC(glyph.nPoints * sizeof(vec2i16));

    _MEMCPY(glyph.flags, tempGlyph.flags, sizeof(TT_POINTFLAG) * glyph.nPoints);
    _MEMCPY(glyph.points, tempGlyph.points, sizeof(vec2i16) * glyph.nPoints);

    // We then release the memory.
    // TODO: Could use arena/slab allocator.

    // So this was the error..
    //StackFree(alloc, glyph.flags);
    //StackFree(alloc, glyph.points);
    _FREE(tempGlyph.flags);
    _FREE(tempGlyph.points);


    return glyph;
}

TT_GLYPHDATA TT_GlyphDataFromCodepoint(TT_FONTINFO *info, u32 codepoint)
{
    u32 index = TT_GlyphIndex(info, codepoint);
    return TT_GlyphDataFromIndex(info, index);
}

u32 TT_GlyphIndex(TT_FONTINFO *info, u32 codepoint)
{
    if (info == NULL || !info->initialized)
    {
        return 0;
    }

    u8* subtable = info->data.data + info->cmap_offset;

    u32 out = 0;

    switch (info->cmap_format)
    {
        case 0:
        {
            if (codepoint > 0xff) return 0;

            out = subtable[6 + codepoint];
        } break;

        case 4:
        {
            if (codepoint > 0xffff) return 0;

            u16 length = TT_READ_BE16(subtable + 2);
            u16 segmentCount = TT_READ_BE16(subtable + 6) / 2;

            i32 seg = -1;

            if (info->cmap_sorted)
            {
                // Sorted cmap.
                i32 low = 0;
                i32 high = (i32) segmentCount - 1;

                // Do some binary search.
                while (low <= high)
                {
                    i32 mid = low + (high - low + 1) / 2;

                    u16 endCode = TT_READ_BE16(subtable + 14 + mid * 2);
                    u16 startCode = TT_READ_BE16(subtable + 16 + 2 * segmentCount + mid * 2);

                    if (codepoint > endCode) {
                        low = mid + 1;
                    } else if (codepoint < startCode) {
                        high = mid - 1;
                    } else {
                        seg = mid;
                        break;
                    }
                }
            }
            else
            {
                // Unsorted cmap.
                for (int i = 0; i < segmentCount; ++i)
                {
                    u16 endCode = TT_READ_BE16(subtable + 14 + i * 2);
                    u16 startCode = TT_READ_BE16(subtable + 16 + 2 * segmentCount + i * 2);

                    if (startCode <= codepoint && codepoint <= endCode)
                    {
                        seg = (i32) i;
                        break;
                    }
                }
            }

            if (seg == -1)
            {
                return 0;
            }

            i16 id_delta = (i16) TT_READ_BE16(subtable + 16 + 4 * segmentCount + seg * 2);
            u16 id_rangeOffset = TT_READ_BE16(subtable + 16+ 6 * segmentCount + seg * 2);

            if (id_rangeOffset == 0)
            {
                out = (u16) ((u16) codepoint + id_delta);
            }
            else
            {
                u16 startCode = TT_READ_BE32(subtable + 16 + 2 * segmentCount + seg * 2);
                u32 numGlyphIndices = (length - (16 + 8 * segmentCount)) / 2;

                u32 glyphIdIndex = (
                    (id_rangeOffset / 2) -
                    (u32) (segmentCount - seg) +
                    (codepoint - startCode)
                );

                if (glyphIdIndex >= numGlyphIndices)
                {
                    return 0;
                }

                u16 index = TT_READ_BE16(subtable + 16 + 8 * segmentCount + glyphIdIndex * 2);

                if (index == 0) {
                    out = index;
                } else {
                    out = (u16) (index + id_delta);
                }
            }
        } break;

        case 6:
        {
            if (codepoint > 0xffff)
            {
                return 0;
            }

            u16 firstCode = TT_READ_BE16(subtable + 6);
            u16 entryCount = TT_READ_BE16(subtable + 8);
            u32 index  = codepoint - firstCode;

            if (index >= entryCount)
            {
                return 0;
            }

            out = TT_READ_BE16(subtable + 10 + index * 2);
        } break;

        case 12:
        case 13:
        {
            u32 numGroups = TT_READ_BE32(subtable + 12);
            i64 group = -1;

            u32 groupOffset = 0, startCode = 0;

            if (info->cmap_sorted)
            {
                // Sorted cmap.
                i16 low = 0;
                i64 high = numGroups - 1;

                while (low <= high)
                {
                    i64 mid = low + (high - low + 1) / 2;

                    groupOffset = 16 + 12 * (u32) mid;
                    startCode = TT_READ_BE32(subtable + groupOffset);
                    u32 endCode = TT_READ_BE32(subtable + groupOffset + 4);

                    if (codepoint > endCode) {
                        low = mid + 1;
                    } else if (codepoint < startCode) {
                        high = mid - 1;
                    } else {
                        group = mid;
                        break;
                    }
                }
            }
            else
            {
                // Unsorted cmap.
                for (int i = 0; i < numGroups; ++i)
                {
                    groupOffset = 16 + 12 * i;
                    startCode = TT_READ_BE32(subtable + groupOffset);
                    u32 endCode = TT_READ_BE16(subtable + groupOffset + 4);

                    if (startCode <= codepoint && codepoint <= endCode)
                    {
                        group = i;
                        break;
                    }
                }
            }

            if (group == -1)
            {
                return 0;
            }

            u32 startIndex = TT_READ_BE32(subtable + groupOffset + 8);
            if (info->cmap_format == 12) {
                out = startIndex + codepoint - startCode;
            } else {
                out = startIndex;
            }
        } break;

        default:
        {
            return 0;
        } break;
    }

    if (out < info->nGlyphs)
    {
        return out;
    }

    return 0;
}

extern u32 TT_CalcChecksum(STRING8 buf, u32 offset, u32 len)
{
    u32 sum = 0;

    for (u32 i = 0; i < len / 4; ++i)
    {
        sum += TT_READ_BE32(buf.data + offset + i * 4);
    }

    u32 leftover = 0;
    for (u32 i = 0; i < len % 4; ++i)
    {
        leftover += (u32) buf.data[offset + (len / 4) * 4 + i] << (3 - i) * 8;
    }

    sum += leftover;

    return sum;
}

extern b32 TT_GetValidateTable(STRING8 buf, u32 tableTag, TT_FONTTABLE* table)
{
    u16 numTables = TT_READ_BE16(buf.data + 4);

    for (int i = 0; i < numTables; ++i)
    {
        u32 recordOffset = 12 + 16 * i;
        u32 tag = TT_READ_BE32(buf.data + recordOffset + 0);
        u32 checksum = TT_READ_BE32(buf.data + recordOffset + 4);
        u32 offset = TT_READ_BE32(buf.data + recordOffset + 8);
        u32 length = TT_READ_BE32(buf.data + recordOffset + 12);

        if (tag != tableTag)
        {
            continue;
        }
        if (offset + length >= buf.len)
        {
            return FALSE;
        }

        u32 realChecksum = TT_CalcChecksum(buf, offset, length);

        // Subtracting head checksumAdjust.
        if (tag == TT_TAG("head"))
        {
            realChecksum -= TT_READ_BE32(buf.data + offset + 8);
        }

        if (checksum != realChecksum)
        {
            return FALSE;
        }

        table->offset = offset;
        table->length = length;

        return TRUE;
    }

    return FALSE;
}

extern b32 TT_ValidateLoca(STRING8 buf, TT_FONTINFO* const info)
{
    TT_FONTTABLE loca = info->loca;

    if (info->loca_format == 0)
    {
        // 16-bit offsets.
        u32 numOffsets = loca.length / sizeof(u16);

        u32 prevOffset = 0;
        for (int i = 0; i < numOffsets; ++i)
        {
            u32 offset = 2 * (u32) TT_READ_BE16(buf.data + i * 2);

            if (offset > info->glyf.length || offset < prevOffset)
            {
                return FALSE;
            }

            prevOffset = offset;
        }
    }
    else
    {
        // 32-bit offsets.
        u32 numOffsets = loca.length / sizeof(u32);

        u32 prevOffset = 0;
        for (int i = 0; i < numOffsets; ++i)
        {
            u32 offset = TT_READ_BE32(buf.data + loca.offset + i * 4);

            if (offset > info->glyf.length || offset < prevOffset)
            {
                return FALSE;
            }

            prevOffset = offset;
        }
    }

    return TRUE;
}

extern b32 TT_FindCmapSubtable(STRING8 buf, TT_FONTINFO* info, TT_FONTTABLE cmap)
{
    if (cmap.length < 4)
    {
        return FALSE;
    }

    u8* cmapData = buf.data + cmap.offset;
    u16 numSubtables = TT_READ_BE16(cmapData + 2);

    // Supported encoding types:
    // Only looking for unicode.
    // Lower indices are preferred (looking for larger ranges of codepoints).
    u32 encodingTypes[] = {
        (0 << 16) | 4,
        (3 << 15) | 10,
        (0 << 16) | 3,
        (0 << 16) | 6,
        (3 << 16) | 1,
        (1 << 16) | 0,
    };
    u32 numEncodingTypes = sizeof(encodingTypes) / sizeof(u32);

    u32 selectedEncodingIndex = UINT32_MAX;
    u32 selectedSubtableOffset = 0;

    if (cmap.length < 4+8*numSubtables)
    {
        return FALSE;
    }

    for (int i = 0; i < numSubtables; ++i)
    {
        u32 subtableOffset = 4 + 8*i;
        u16 platformId = TT_READ_BE16(cmapData + subtableOffset + 0);
        u16 encodingId = TT_READ_BE16(cmapData + subtableOffset + 2);
        u32 offset = TT_READ_BE32(cmapData + subtableOffset + 4);

        if (offset > cmap.length)
        {
            return FALSE;
        }

        u32 encodingIndex = UINT32_MAX;
        for (int j = 0; j < numEncodingTypes; ++j)
        {
            if ((((u32) platformId << 16) | encodingId) == encodingTypes[j])
            {
                encodingIndex = j;
                break;
            }
        }

        // Preferring smaller indices.
        if (encodingIndex < selectedEncodingIndex)
        {
            selectedEncodingIndex = encodingIndex;
            selectedSubtableOffset = offset;
        }
    }

    if (selectedEncodingIndex == UINT32_MAX)
    {
        return FALSE;
    }

    u32 cmapOffset = cmap.offset + selectedSubtableOffset;
    u32 localOffset = selectedSubtableOffset;

    if (cmap.length < localOffset + 2)
    {
        return FALSE;
    }

    u16 format = TT_READ_BE16(buf.data + cmapOffset);

    info->cmap_sorted = TRUE;

    // Validating ofrmat, length and indices within subtable/
    // Not parsing any glyph indices.
    switch (format)
    {
        case 0:
        {
            u32 requiredLen = 6 + 256;
            if (cmap.length < localOffset + requiredLen)
            {
                return FALSE;
            }


            u16 length = TT_READ_BE16(buf.data + cmapOffset + 2);
            if (length < requiredLen)
            {
                return FALSE;
            }
        } break;

        case 4:
        {
            if (cmap.length < localOffset + 20)
            {
                return FALSE;
            }

            u16 length = TT_READ_BE16(buf.data + cmapOffset + 2);
            u16 segCount = TT_READ_BE16(buf.data + cmapOffset + 6) / 2;
            u32 minLength = 20 + (u32) segCount + 4 * sizeof(u16);
            if (length < minLength || cmap.length < length)
            {
                return FALSE;
            }


            u16 prevEnd = 0;
            for (int i = 0; i < segCount; ++i)
            {
                u16 endCode = TT_READ_BE16(buf.data + cmapOffset + 14 + i * 2);
                u16 startCode = TT_READ_BE16(buf.data + cmapOffset + 16 + segCount * 2 + i * 2);
                if (endCode < startCode)
                {
                    return FALSE;
                }
                if (endCode < prevEnd)
                {
                    info->cmap_sorted = FALSE;
                }


                prevEnd = endCode;
            }
        } break;

        case 6:
        {
            if (cmap.length < localOffset + 10)
            {
                return FALSE;
            }

            u16 length = TT_READ_BE16(buf.data + cmapOffset + 2);
            u16 entryCount = TT_READ_BE16(buf.data + cmapOffset + 8);
            u16 impliedLength = 10 + entryCount * 2;

            if (length < impliedLength || cmap.length < localOffset + length)
            {
                return FALSE;
            }
        } break;

        case 12:
        case 13:
        {
            if (cmap.length < localOffset + 16)
            {
                return FALSE;
            }

            u32 length = TT_READ_BE32(buf.data + cmapOffset + 4);
            u32 numGroups = TT_READ_BE32(buf.data + cmapOffset + 12);
            u32 impliedLength = 16 + numGroups*12;

            if (length < impliedLength || cmap.length < localOffset + length)
            {
                return FALSE;
            }

            u32 prevEnd = 0;
            for (int i = 0; i < numGroups; ++i)
            {
                u32 groupOffset = 16 + 12*i;
                u32 startCode = TT_READ_BE32(buf.data + cmapOffset + groupOffset);
                u32 endCode = TT_READ_BE32(buf.data + cmapOffset + groupOffset + 4);

                if (endCode < startCode)
                {
                    return FALSE;
                }
                if (endCode < prevEnd)
                {
                    info->cmap_sorted = FALSE;
                }

                prevEnd = endCode;
            }
        } break;

        default:
        {
            return FALSE;
        } break;
    }

    info->cmap_offset = cmapOffset;
    info->cmap_format = format;

    return TRUE;
}

extern TT_GLYFENTRY TT_FindGlyfEntry(TT_FONTINFO* info, u32 glyphIndex)
{
    if (glyphIndex >= info->nGlyphs)
    {
        return CLITERAL(TT_GLYFENTRY) { 0 };
    }

    // The font info already has the buffer,
    // so we don't have to explicitly pass it
    // in the function.
    u8* loca = info->data.data + info->loca.offset;


    u32 offset = 0;
    u32 nextOffset = 0;
    if (info->loca_format == 0)
    {
        offset = (u32) TT_READ_BE16(loca + glyphIndex * 2) * 2;
        nextOffset = (u32) TT_READ_BE16(loca + (glyphIndex + 1) * 2) * 2;
    }
    else if (info->loca_format == 1)
    {
        offset = TT_READ_BE32(loca + glyphIndex * 4);
        nextOffset = TT_READ_BE32(loca + (glyphIndex + 1) * 4);
    }

    if (nextOffset > info->glyf.length)
    {
        return CLITERAL(TT_GLYFENTRY) { 0 };
    }


    return CLITERAL(TT_GLYFENTRY)
    {
        offset,
        nextOffset - offset,
    };
}


extern float TT_ScaleForPixelHeight(STRING8 buf, TT_FONTINFO* const info, float height)
{
    int fheight = TT_READ_BE16(buf.data + info->head.offset + 4) - TT_READ_BE16(buf.data + info->head.offset + 6);
    fprintf(stderr, "%s: height: %d\n", __func__, fheight);
    return (float) height / fheight;
}
