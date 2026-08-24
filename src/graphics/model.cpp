#include "model.h"

#include <string.h>
#include <assert.h>
#include "../base/mem_ds.h"

#define MESH_SHADING_FLAT (0)
#define MESH_SHADING_SMOOTH (1)

// Smooth shading usually involves turning the triangle mesh into a graph
// (for the adjacent vertices) then finding the average of the normals,
// in order to create the illusion of continuity.
// The original normals are the triangle flat shaded normals.
// Many of the same vertices can share the same position.

/*
    fprintf(stderr, "%s: nfaces:%d, vtxpos:%p, idxpos:%p\n", __func__, nfaces, vtx.pos, idx.pos);
    fprintf(stderr, "%s: num vtxpos:%d, num idxpos:%d\n", __func__, vtx.npos, idx.npos);
*/

static int MeshSetNormalsFlat(MESH* mesh, VERTEXBUFFER vtx, INDEXBUFFER idx)
{
    // TODO: Issue.

    //return 0;

    // Set on triangles.
    /*
    int idxcount = idx.npos;

    for (int i = 0; i < idxcount; i+=3)
    {
        //fprintf(stderr, "tri(%d,%d,%d)\n", idx.pos[i+0], idx.pos[i+1], idx.pos[i+2]);

        // Get 3 points in a triangle.
        vec3 a = vtx.pos[idx.pos[i+0]];
        vec3 b = vtx.pos[idx.pos[i+1]];
        vec3 c = vtx.pos[idx.pos[i+2]];

        vec3 edge1 = b - a;
        vec3 edge2 = c - a;

        // Compute cross product and fill in the data.
        vec3 normal = vec3::Cross(edge1, edge2);
        normal = normal.Nor();
        //vec3 normal = edge1;

        //printf("nidx(%d %d %d) ", idx.normal[i+0], idx.normal[i+1], idx.normal[i+2]);

        // For flat shading, all the vertices share the same normal.
        vtx.normal[idx.normal[i+0]] = normal;
        vtx.normal[idx.normal[i+1]] = normal;
        vtx.normal[idx.normal[i+2]] = normal;

        //printf("nidx(%d %d %d) ", idx.normal[i+0], idx.normal[i+1], idx.normal[i+2]);
        //PrintVec3(vtx.normal[idx.normal[i+0]]);
    }
    */
    //printf("\n");

    return 0;
}

extern int MeshBuild(MESH* mesh,
    VERTEXBUFFER vtx,
    INDEXBUFFER idx, b32 dirty)
{
    fprintf(stderr, "%s: Ok now we're calling ts.\n", __func__);
    /*
    // Find bounding box of mesh.
    vec3 bboxMin = CLITERAL(vec3) { +1e6, +1e6, +1e6 };
    vec3 bboxMax = CLITERAL(vec3) { -1e6, -1e6, -1e6 };

    // This is the vertex assembly
    // stage of the pipeline.
    int idxcount = idx.npos;
    int numFaces = idxcount / 3;

    // We do checks.
    assert(numFaces > 0 && numFaces * 3 == idx.npos);
    assert(idx.npos == idxcount);
    assert(idx.nnormal == idxcount);
    assert(idx.ntexcoords == idxcount);
    // FAILED there.
    //assert(idx.ncol == idxcount);

    fprintf(stderr, "%s: initial idxcount=%d\n", __func__, (int) idxcount);

    vertex_t* vertices = (vertex_t *) _MALLOC(sizeof(vertex_t) * idxcount);
    _MEMSET(vertices, 0, sizeof(vertex_t) * idxcount);
    fprintf(stderr, "%s: idxcount=%d\n", __func__, (int) idxcount);

    // Copy indices to vertices.
    for (size_t i = 0; i < idxcount; ++i) {
        int pindex = idx.pos[i];
        int texcoordindex = idx.texcoords[i];
        int normalindex = idx.normal[i];

        //fprintf(stderr, "%s: pindex:%d\n", __func__, pindex); 
        //fprintf(stderr, "%s: texcoordindex:%d\n", __func__, texcoordindex);
        //fprintf(stderr, "%s: normalindex:%d\n", __func__, normalindex);

        // We do checks.
        assert(pindex >= 0 && pindex <= vtx.npos);
        assert(texcoordindex >= 0 && texcoordindex <= vtx.ntexcoords);
        assert(normalindex >= 0 && normalindex <= vtx.nnormal);

        // Required vertex attributes.
        vertices[i].pos = vtx.pos[pindex];
        vertices[i].texcoords = vtx.texcoords[texcoordindex];
        vertices[i].normal = vtx.normal[normalindex];

        // Optional vertex attributes (uses position indices only).
        if (vtx.tangents) {
            int tgindex = pindex;
            assert(tgindex >= 0 && tgindex <= vtx.ntangents);
            vertices[i].tangent = vtx.tangents[tgindex];
        } else {
            vertices[i].tangent = CLITERAL(vec4) { 1, 0, 0, 1 };
        }

        if (vtx.joints) {
            int jointIndex = pindex;
            assert(jointIndex >= 0 && jointIndex <= vtx.njoint);
            vertices[i].joint = vtx.joints[jointIndex];
        } else {
            vertices[i].joint = CLITERAL(vec4) { 0,0, 0, 0 };
        }

        if (vtx.weights) {
            int weightIndex = pindex;
            assert(weightIndex >= 0 && weightIndex <= vtx.nweight);
            vertices[i].weight = vtx.weights[weightIndex];
        } else {
            vertices[i].weight = CLITERAL(vec4) { 0, 0, 0, 0 };
        }

        if (vtx.col) {
            int colindex = pindex;
            assert(colindex >= 0 && colindex <= vtx.ncol);

            vec3 col = vtx.col[colindex];
            vertices[i].col = CLITERAL(vec4) { col.x, col.y, col.z, 1.0f };
        } else {
            vertices[i].col = CLITERAL(vec4) { 1, 1, 1, 1 };
        }

        // Calculate min and max bounding box.
        Vec3Min(&bboxMin, &bboxMin, &vertices[i].pos);
        Vec3Max(&bboxMax, &bboxMax, &vertices[i].pos);
    }
    
    vec3 sum; Vec3Add(&sum, &bboxMin, &bboxMax);
    Vec3DivScalar(&sum, &sum, 2.0f);

    mesh->nfaces = numFaces;
    mesh->vertices = vertices;
    mesh->indices = idx.pos;

    mesh->center = sum;
    mesh->nvertices = vtx.npos;
    mesh->vertexBuffer = NULL;
    mesh->indexBuffer = NULL;

    fprintf(stderr, "%s: Nfaces: %d, vertices: %p\n", __func__, mesh->nfaces, mesh->vertices);

    fprintf(stderr, "%s: Done.\n", __func__);
    */

    return 0;
}

#define LINE_SIZE 128
#define MESH_INIT_CAP 100

DECL_DA(PosVec, vec3)
DECL_DA(TexcoordVec, vec2);
DECL_DA(NorVec, vec3);
DECL_DA(TanVec, vec4);
DECL_DA(JointVec, vec4);
DECL_DA(WeightVec, vec4);

DECL_DA(IndexVec, int);


extern int MeshLoad_OBJ(MESH* mesh, const char* path)
{
    // TODO: Fix.
    /*
    PosVec positions = { 0 };
    TexcoordVec texcoords = { 0 };
    NorVec normals = { 0 };
    TanVec tangents = { 0 };
    JointVec joints = { 0 };
    WeightVec weights = { 0 };

    IndexVec pos_indices = { 0 };
    IndexVec texcoord_indices = { 0 };
    IndexVec normal_indices = { 0 };

    DA_Reserve(&positions, MESH_INIT_CAP);
    DA_Reserve(&texcoords, MESH_INIT_CAP);
    DA_Reserve(&normals, MESH_INIT_CAP);
    DA_Reserve(&tangents, MESH_INIT_CAP);
    DA_Reserve(&joints, MESH_INIT_CAP);
    DA_Reserve(&weights, MESH_INIT_CAP);

    DA_Reserve(&pos_indices, MESH_INIT_CAP);
    DA_Reserve(&texcoord_indices, MESH_INIT_CAP);
    DA_Reserve(&normal_indices, MESH_INIT_CAP);

    b32 dirty = FALSE;

    fprintf(stderr, "%s: Weight capacity:%d\n", __func__, (int) weights.capacity);


    char line[LINE_SIZE];

    FILE* file;
    file = fopen(path, "rb");
    assert(file != NULL);
    
    while (1) {
        int items;
        if (fgets(line, LINE_SIZE, file) == NULL) {
            // We've reached the EOL.
            fprintf(stderr, "%s: EOL reached!\n", __func__);
            break;
        } else if (strncmp(line, "v ", 2) == 0) {
            // Position.
            vec3 pos;
            items = sscanf(line, "v %f %f %f",
                    &pos.x, &pos.y, &pos.z);
            assert(items == 3);

            DA_Push(&positions, pos);
        } else if (strncmp(line, "vt ", 3) == 0) {
            // Texture.
            vec2 texcoord;
            items = sscanf(line, "vt %f %f",
                    &texcoord.x, &texcoord.y);
            assert(items == 2);

            DA_Push(&texcoords, texcoord);
        } else if (strncmp(line, "vn ", 3) == 0) {
            vec3 normal;
            items = sscanf(line, "vn %f %f %f",
                    &normal.x, &normal.y, &normal.z);
            assert(items == 3);

            DA_Push(&normals, normal);
        } else if (strncmp(line, "f ", 2) == 0) {
            int posidx[3], uvidx[3], nidx[3];
            items = sscanf(line, "f %d/%d/%d, %d/%d/%d, %d/%d/%d",
                &posidx[0], &uvidx[0], &nidx[0],
                &posidx[1], &uvidx[1], &nidx[1],
                &posidx[2], &uvidx[2], &nidx[2]);
            if (items != 9) {
                // Fallback to simpler format.
                items = sscanf(line, "f %d %d %d", &posidx[0], &posidx[1], &posidx[2]);

                for (int i = 0; i < 3; ++i) {
                    uvidx[i] = posidx[i];
                    nidx[i] = posidx[i];
                }
                dirty = TRUE;
            }

            for (int i = 0; i < 3; ++i) {
                DA_Push(&pos_indices, posidx[i] - 1);
                DA_Push(&texcoord_indices, uvidx[i] - 1);
                DA_Push(&normal_indices, nidx[i] - 1);
            }
        }

        // Extensions.
        else if (strncmp(line, "# ext.tangent ", 14) == 0) {
            vec4 tangent;
            items = sscanf(line, "# ext.tangent %f %f %f %f",
                &tangent.x, &tangent.y, &tangent.z, &tangent.w);
            assert(items == 4);

            DA_Push(&tangents, tangent);
        } else if (strncmp(line, "# ext.joint ", 12) == 0) {
            vec4 joint;
            items = sscanf(line, "# ext.joint %f %f %f %f",
                &joint.x, &joint.y, &joint.z, &joint.w);
            assert(items == 4);

            DA_Push(&joints, joint);
        } else if (strncmp(line, "# ext.weight ", 13) == 0) {
            vec4 weight;
            items = sscanf(line, "# ext.weight %f %f %f %f",
                &weight.x, &weight.y, &weight.z, &weight.w);
            assert(items == 4);

            DA_Push(&weights, weight);
        }
    }
    fclose(file);

    // Might do some normal calculations if dirty.

    VERTEXBUFFER vtx;
    INDEXBUFFER idx;
    _MEMSET(&vtx, 0, sizeof(vtx));
    _MEMSET(&idx, 0, sizeof(idx));
    vtx.pos = positions.items;
    vtx.texcoords = texcoords.items;
    vtx.normal = normals.items;
    vtx.tangents = tangents.items;
    vtx.joints = joints.items;
    vtx.weights = weights.items;

    vtx.npos = DA_Count(positions);

    if (dirty) {
        vtx.ntexcoords = vtx.npos;
        vtx.nnormal = vtx.npos;

        vtx.tangents = vtx.joints = vtx.weights = NULL;
    } else {
        vtx.ntexcoords = DA_Count(texcoords);
        vtx.nnormal = DA_Count(normals);

        vtx.ntangents = DA_Count(tangents);
        vtx.njoint = DA_Count(joints);
        vtx.nweight = DA_Count(weights);
    }


    idx.pos = pos_indices.items;
    idx.texcoords = texcoord_indices.items;
    idx.normal = normal_indices.items;
    idx.tangents = idx.joints = idx.weights = NULL;
    
    idx.npos = DA_Count(pos_indices);
    idx.ntexcoords = DA_Count(texcoord_indices);
    idx.nnormal = DA_Count(normal_indices);
    idx.ntangents = idx.njoint = idx.nweight = 0;

    DA_Reserve(&normals, positions.capacity);
    DA_Reserve(&texcoords, positions.capacity);

    fprintf(stderr, "%s: vtx pos=%p  npos=%d  cap=%d\n", __func__, vtx.pos, vtx.npos, (int) positions.capacity);
    fprintf(stderr, "%s: vtx normal=%p  nnormal=%d  cap=%d\n", __func__, vtx.normal, vtx.nnormal, (int) normals.capacity);
    fprintf(stderr, "%s: vtx texcoords=%p  ntexcoords=%d  cap=%d\n", __func__, vtx.texcoords, vtx.ntexcoords, (int) texcoords.capacity);

    fprintf(stderr, "%s: idx pos=%p  npos=%d  cap=%d\n", __func__, idx.pos, idx.npos, (int) pos_indices.capacity);
    fprintf(stderr, "%s: idx normal=%p  nnormal=%d  cap=%d\n", __func__, idx.normal, idx.nnormal, (int) normal_indices.capacity);
    fprintf(stderr, "%s: idx texcoords=%p  ntexcoords=%d  cap=%d\n", __func__, idx.texcoords, idx.ntexcoords, (int) texcoord_indices.capacity);

    // We reserve capacity for normals and texture coordinates.
    if (dirty) {
        fprintf(stderr, "Hit dirty flag!\n");
        MeshSetNormalsFlat(mesh, vtx, idx);
    }

    MeshBuild(mesh, vtx, idx, dirty);
    */

    // And then we free the dynamic arrays.
    // TODO: Ok. Maybe not all of them.
    /*
    DA_Free(&positions);
    DA_Free(&texcoords);
    DA_Free(&normals);
    DA_Free(&tangents);
    DA_Free(&joints);
    DA_Free(&weights);

    DA_Free(&pos_indices);
    DA_Free(&texcoord_indices);
    DA_Free(&normal_indices);
    */


    fprintf(stderr, "[ERROR] %s: Failed afterwards! (brruuhhh)\n", __func__);

    return 0;
}


extern void MeshTerminate(MESH* mesh)
{
    fprintf(stderr, "%s: Terminating mesh!\n", __func__);
    if (mesh->vertices != NULL) _FREE(mesh->vertices);
    if (mesh->indices != NULL) _FREE(mesh->indices);

    _MEMSET(mesh, 0, sizeof(*mesh));
}
