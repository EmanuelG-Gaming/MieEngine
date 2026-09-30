#include "mesh.h"
#include "../base/base_log.h"

#include "draw.h"


extern int MeshBuilderInit(MeshBuilder* mesh, ARENA* arena)
{
    //mesh->nVertices = vertexCount;
    mesh->nVertices = -1;
    mesh->vertexCounter = 0;

    mesh->arena = arena;
    mesh->memOffset = 0;

    mesh->positions = NULL;
    mesh->colors = NULL;
    mesh->texCoords = NULL;
    mesh->indices = NULL;

    return 0;
}

extern int MeshBuilderReserve(MeshBuilder* mesh, int vertexCount, int indexCount)
{
    if (mesh->arena != NULL)
    {
        ArenaPop(mesh->arena, mesh->memOffset);

        mesh->positions = ArenaPushArrayZero(mesh->arena, float, vertexCount * 3);
        mesh->colors = ArenaPushArrayZero(mesh->arena, float, vertexCount * 4);
        mesh->texCoords = ArenaPushArrayZero(mesh->arena, float, vertexCount * 2);
        mesh->indices = ArenaPushArrayZero(mesh->arena, unsigned int, indexCount);

        mesh->memOffset = vertexCount * (3+4+2) * sizeof(float);
        mesh->nVertices = vertexCount;
        mesh->nIndices = indexCount;

        return 0;
    }

    return -1;
}

extern void MeshBuilderTerminate(MeshBuilder* mesh)
{
    if (mesh->arena != NULL)
    {
        ArenaPop(mesh->arena, mesh->memOffset);
        mesh->memOffset = 0;
        mesh->nVertices = 0;

        mesh->positions = NULL;
        mesh->colors = NULL;
        mesh->texCoords = NULL;

        mesh->arena = NULL;
    }
}

extern void MeshPushVertex3RGBAUV(MeshBuilder* mesh,
    float x, float y, float z,
    float r, float g, float b, float a,
    float u, float v)
{
    float* pos = mesh->positions + mesh->vertexCounter*3;
    float* color = mesh->colors + mesh->vertexCounter*4;
    float* texcoord = mesh->texCoords + mesh->vertexCounter*2;

    pos[0] = x;
    pos[1] = y;
    pos[2] = z;

    color[0] = r;
    color[1] = g;
    color[2] = b;
    color[3] = a;

    texcoord[0] = u;
    texcoord[1] = v;

    mesh->vertexCounter++;
}

static void MeshPushIndices(MeshBuilder* mesh, unsigned int const* indices, int nIndices)
{
    for (int i = 0; i < nIndices; ++i)
    {
        mesh->indices[i] = indices[i];
    }
}


extern MeshBuilder MeshCreateCuboid(ARENA* arena, float w, float h, float d)
{
    MeshBuilder mesh;
    MeshBuilderInit(&mesh, arena);
    MeshBuilderReserve(&mesh, 8, 36);

    w = w*0.5f;
    h = h*0.5f;
    d = d*0.5f;

    // Bottom cap
    MeshPushVertex3RGBAUV(&mesh, -w, -h, -d, 1, 1, 1, 1, 0, 0);
    MeshPushVertex3RGBAUV(&mesh, -w, -h, d, 1, 1, 1, 1, 0, 1);
    MeshPushVertex3RGBAUV(&mesh, w, -h, d, 1, 1, 1, 1, 1, 1);
    MeshPushVertex3RGBAUV(&mesh, w, -h, -d, 1, 1, 1, 1, 1, 0);

    // Top cap.
    MeshPushVertex3RGBAUV(&mesh, -w, h, -d, 1, 1, 1, 1, 0, 0);
    MeshPushVertex3RGBAUV(&mesh, -w, h, d, 1, 1, 1, 1, 0, 1);
    MeshPushVertex3RGBAUV(&mesh, w, h, d, 1, 1, 1, 1, 1, 1);
    MeshPushVertex3RGBAUV(&mesh, w, h, -d, 1, 1, 1, 1, 1, 0);

    // Push indices.
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3,

        4, 6, 5,
        4, 7, 6,

        4, 5, 1,
        4, 1, 0,

        3, 2, 6,
        3, 6, 7,

        1, 5, 6,
        1, 6, 2,

        4, 0, 3,
        4, 3, 7,
    };

    // Copy indices.
    MeshPushIndices(&mesh, indices, 36);



    // 6 vertices per quad
    // x, y, z, r,g,b,a, u,v.
    // Bottom (-h)
    /*
    MeshPushVertex3RGBAUV(&mesh, -w, -h, -d, 1, 1, 1, 1, 0, 0);
    MeshPushVertex3RGBAUV(&mesh, w, -h, -d, 1, 1, 1, 1, 1, 0);
    MeshPushVertex3RGBAUV(&mesh, w, -h, d, 1, 1, 1, 1, 1, 1);
    MeshPushVertex3RGBAUV(&mesh, -w, -h, -d, 1, 1, 1, 1, 0, 0);
    MeshPushVertex3RGBAUV(&mesh, w, -h, d, 1, 1, 1, 1, 1, 1);
    MeshPushVertex3RGBAUV(&mesh, -w, -h, d, 1, 1, 1, 1, 0, 1);

    // Top (+h)
    MeshPushVertex3RGBAUV(&mesh, -w, h, -d, 1, 1, 1, 1, 0, 0);
    MeshPushVertex3RGBAUV(&mesh, w, h, -d, 1, 1, 1, 1, 1, 0);
    MeshPushVertex3RGBAUV(&mesh, w, h, d, 1, 1, 1, 1, 1, 1);
    MeshPushVertex3RGBAUV(&mesh, -w, h, -d, 1, 1, 1, 1, 0, 0);
    MeshPushVertex3RGBAUV(&mesh, w, h, d, 1, 1, 1, 1, 1, 1);
    MeshPushVertex3RGBAUV(&mesh, -w, h, d, 1, 1, 1, 1, 0, 1);

    // Back (-d)
    MeshPushVertex3RGBAUV(&mesh, -w, -h, -d, 1, 1, 1, 1, 0, 0);
    MeshPushVertex3RGBAUV(&mesh, w, -h, -d, 1, 1, 1, 1, 1, 0);
    MeshPushVertex3RGBAUV(&mesh, w, h, -d, 1, 1, 1, 1, 1, 1);
    MeshPushVertex3RGBAUV(&mesh, -w, -h, -d, 1, 1, 1, 1, 0, 0);
    MeshPushVertex3RGBAUV(&mesh, w, h, -d, 1, 1, 1, 1, 1, 1);
    MeshPushVertex3RGBAUV(&mesh, -w, h, -d, 1, 1, 1, 1, 0, 1);

    // Front (+d)
    MeshPushVertex3RGBAUV(&mesh, -w, -h, d, 1, 1, 1, 1, 0, 0);
    MeshPushVertex3RGBAUV(&mesh, w, -h, d, 1, 1, 1, 1, 1, 0);
    MeshPushVertex3RGBAUV(&mesh, w, h, d, 1, 1, 1, 1, 1, 1);
    MeshPushVertex3RGBAUV(&mesh, -w, -h, d, 1, 1, 1, 1, 0, 0);
    MeshPushVertex3RGBAUV(&mesh, w, h, d, 1, 1, 1, 1, 1, 1);
    MeshPushVertex3RGBAUV(&mesh, -w, h, d, 1, 1, 1, 1, 0, 1);

    // Left (-w)
    MeshPushVertex3RGBAUV(&mesh, -w, -h, -d, 1, 1, 1, 1, 0, 0);
    MeshPushVertex3RGBAUV(&mesh, -w, -h, d, 1, 1, 1, 1, 1, 0);
    MeshPushVertex3RGBAUV(&mesh, -w, h, d, 1, 1, 1, 1, 1, 1);
    MeshPushVertex3RGBAUV(&mesh, -w, -h, -d, 1, 1, 1, 1, 0, 0);
    MeshPushVertex3RGBAUV(&mesh, -w, h, d, 1, 1, 1, 1, 1, 1);
    MeshPushVertex3RGBAUV(&mesh, -w, h, d, 1, 1, 1, 1, 0, 1);

    // Right (+w)
    MeshPushVertex3RGBAUV(&mesh, w, -h, -d, 1, 1, 1, 1, 0, 0);
    MeshPushVertex3RGBAUV(&mesh, w, -h, d, 1, 1, 1, 1, 1, 0);
    MeshPushVertex3RGBAUV(&mesh, w, h, d, 1, 1, 1, 1, 1, 1);
    MeshPushVertex3RGBAUV(&mesh, w, -h, -d, 1, 1, 1, 1, 0, 0);
    MeshPushVertex3RGBAUV(&mesh, w, h, d, 1, 1, 1, 1, 1, 1);
    MeshPushVertex3RGBAUV(&mesh, w, h, -d, 1, 1, 1, 1, 0, 1);
    */




    return mesh;
}



#define D3D_ARGB(a, r, g, b) \
    ((u32)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))



/*
     The memory structure was:
                                                   arena ptr
                                                       v
     [positions][colors][texture coordinates][vertices]



     And now it's: (after ArenaTempEnd)
                                         arena ptr
                                             v
     [positions][colors][texture coordinates][vertices]



     And then: (after MeshBuilderTerminate)
     arena ptr
     v
     [positions][colors][texture coordinates][vertices]


     We overwrite data by adding in our new GFX_mesh.
           arena ptr
               v
     [gfx mesh][rest of positions][colors][texture coordinates][vertices]
*/


extern GFX_mesh* MeshBuilderCreateMesh(MeshBuilder* mesh, ARENA* uploadTo)
{
    //ARENA_TEMP temp = ArenaTempBegin(mesh->arena);

    // Push vertices.
    CustomVertex* output = ArenaPushArrayZero(mesh->arena, CustomVertex, mesh->nVertices);

    // Iterate over vertices.
    for (int i = 0; i < mesh->nIndices; i+=3)
    {
        // Iterate over triangles.

        // Yanderedev ahh code.
        int idx1 = mesh->indices[i+0];
        int idx2 = mesh->indices[i+1];
        int idx3 = mesh->indices[i+2];

        float* pos1 = mesh->positions + (idx1 * 3);
        float* color1 = mesh->colors + (idx1 * 4);
        float* tex1 = mesh->texCoords + (idx1 * 2);

        float* pos2 = mesh->positions + (idx2 * 3);
        float* color2 = mesh->colors + (idx2 * 4);
        float* tex2 = mesh->texCoords + (idx2 * 2);

        float* pos3 = mesh->positions + (idx3 * 3);
        float* color3 = mesh->colors + (idx3 * 4);
        float* tex3 = mesh->texCoords + (idx3 * 2);



        CustomVertex* vtx1 = output + (i+0);
        CustomVertex* vtx2 = output + (i+1);
        CustomVertex* vtx3 = output + (i+2);


        vtx1->x = pos1[0];
        vtx1->y = pos1[1];
        vtx1->z = pos1[2];
        //vertices->rhw = 0.0f;

        vtx1->u = tex1[0];
        vtx1->v = tex1[1];

        // And then the massive calculation.
        // 0xAARRGGBB format.
        vtx1->color = D3D_ARGB(
            (int) (color1[3]*255.0f),
            (int) (color1[0]*255.0f),
            (int) (color1[1]*255.0f),
            (int) (color1[2]*255.0f));

        // 2nd vertex.
        vtx2->x = pos2[0];
        vtx2->y = pos2[1];
        vtx2->z = pos2[2];

        vtx2->u = tex2[0];
        vtx2->v = tex2[1];

        vtx2->color = D3D_ARGB(
            (int) (color2[3]*255.0f),
            (int) (color2[0]*255.0f),
            (int) (color2[1]*255.0f),
            (int) (color2[2]*255.0f));

        // 3rd vertex.
        vtx3->x = pos3[0];
        vtx3->y = pos3[1];
        vtx3->z = pos3[2];

        vtx3->u = tex3[0];
        vtx3->v = tex3[1];

        vtx3->color = D3D_ARGB(
            (int) (color3[3]*255.0f),
            (int) (color3[0]*255.0f),
            (int) (color3[1]*255.0f),
            (int) (color3[2]*255.0f));
    }
    // Then print those vertices for testing.
    /*
    for (int i = 0; i < mesh->nVertices; ++i)
    {
        CustomVertex v = out[i];

        LogInfoEmitF("vertex: %f %f %f %f %f\n", v.x, v.y, v.z, v.u, v.v);
    }
    */


    //ArenaTempEnd(temp);
    // Also deallocate the mesh builder buffers along the way.
    //MeshBuilderTerminate(mesh);

    GFX_mesh* res = DrawUploadMesh(uploadTo, output, mesh->nVertices);


    return res;
}

