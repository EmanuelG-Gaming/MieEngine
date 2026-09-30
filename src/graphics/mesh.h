#ifndef MESH_H_
#define MESH_H_ 1

#include "../mem/arena.h"
#include "draw.h"

/*
   Mesh generation utilities.
*/


#define MESH_BUILDER_INDEXED 1

typedef struct MeshBuilder {
    ARENA* arena;

    float* positions;
    float* colors;
    float* texCoords;
    unsigned int* indices;

    int nVertices;
    int nIndices;
    int vertexCounter;
    int memOffset;

    int flags;
} MeshBuilder;

extern int MeshBuilderInit(MeshBuilder* mesh, ARENA* arena);
extern void MeshBuilderTerminate(MeshBuilder* mesh);
extern int MeshBuilderReserve(MeshBuilder* mesh, int vertexCount, int indexCount);


extern void MeshPushVertex3RGBAUV(MeshBuilder* mesh,
    float x, float y, float z,
    float r, float g, float b, float a,
    float u, float v);

extern MeshBuilder MeshCreateCuboid(ARENA* arena, float w, float h, float d);


extern GFX_mesh* MeshBuilderCreateMesh(MeshBuilder* mesh, ARENA* uploadTo);



#endif /* MESH_H_ */
