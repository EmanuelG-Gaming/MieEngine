#include "physics.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifdef PHE_USE_FIXED_POINT
#define PHE_RADIUS_EPSILON 1
#else
#define PHE_RADIUS_EPSILON 1e-6
#endif

#define PHE_WORLD_CAPACITY 1024
#define PHE_WORLD_MANIFOLD_CAPACITY 256

// Q16.16 format.
#ifdef PHE_USE_FIXED_POINT
#define PHE_DEFAULT_GRAVITY 0x0009CF5C
#else
#define PHE_DEFAULT_GRAVITY 9.81f
#endif





/*
   Vector utilities.
*/

#ifdef PHE_USE_FIXED_POINT

// Fixed-point arithmetic (Q16.16).

/*
   Scalars.
*/

static inline scalar PHE_scalarAdd(scalar a, scalar b)
{
    return fix16_add(a, b);
}
static inline scalar PHE_scalarSub(scalar a, scalar b)
{
    return fix16_sub(a, b);
}
static inline scalar PHE_scalarMul(scalar a, scalar b)
{
    return fix16_mul(a, b);
}
static inline scalar PHE_scalarDiv(scalar a, scalar b)
{
    return fix16_div(a, b);
}



/*
   Vectors.
*/
static inline PHE_VEC3* PHE_vec3Add(PHE_VEC3* out, const PHE_VEC3* a, const PHE_VEC3* b)
{
    out->x = fix16_add(a->x, b->x);
    out->y = fix16_add(a->y, b->y);
    out->z = fix16_add(a->z, b->z);
    return out;
}
static inline PHE_VEC3* PHE_vec3Sub(PHE_VEC3* out, const PHE_VEC3* a, const PHE_VEC3* b)
{
    out->x = fix16_sub(a->x, b->x);
    out->y = fix16_sub(a->y, b->y);
    out->z = fix16_sub(a->z, b->z);
    return out;
}
static inline PHE_VEC3* PHE_vec3Mul(PHE_VEC3* out, const PHE_VEC3* a, const PHE_VEC3* b)
{
    out->x = fix16_mul(a->x, b->x);
    out->y = fix16_mul(a->y, b->y);
    out->z = fix16_mul(a->z, b->z);
    return out;
}
static inline PHE_VEC3* PHE_vec3Div(PHE_VEC3* out, const PHE_VEC3* a, const PHE_VEC3* b)
{
    out->x = fix16_div(a->x, b->x);
    out->y = fix16_div(a->y, b->y);
    out->z = fix16_div(a->z, b->z);
    return out;
}

// Scalars.
static inline PHE_VEC3* PHE_vec3MulScalar(PHE_VEC3* out, const PHE_VEC3* a, scalar scl)
{
    out->x = fix16_mul(a->x, scl);
    out->y = fix16_mul(a->y, scl);
    out->z = fix16_mul(a->z, scl);
    return out;
}
static inline PHE_VEC3* PHE_vec3DivScalar(PHE_VEC3* out, const PHE_VEC3* a, scalar scl)
{
    out->x = fix16_div(a->x, scl);
    out->y = fix16_div(a->y, scl);
    out->z = fix16_div(a->z, scl);
    return out;
}


#else /* PHE_USE_FIXED_POINT */

/*
   Wrappers for
   Floating-point arithmetic (IEE-754).
*/

/*
   Scalars.
*/

static inline scalar PHE_scalarAdd(scalar a, scalar b)
{
    return (a + b);
}
static inline scalar PHE_scalarSub(scalar a, scalar b)
{
    return (a - b);
}
static inline scalar PHE_scalarMul(scalar a, scalar b)
{
    return (a * b);
}
static inline scalar PHE_scalarDiv(scalar a, scalar b)
{
    return (a / b);
}


/*
   Vectors.
*/

static inline PHE_VEC3* PHE_vec3Add(PHE_VEC3* out, const PHE_VEC3* a, const PHE_VEC3* b)
{
    return Vec3Add(out, a, b);
}
static inline PHE_VEC3* PHE_vec3Sub(PHE_VEC3* out, const PHE_VEC3* a, const PHE_VEC3* b)
{
    return Vec3Sub(out, a, b);
}
static inline PHE_VEC3* PHE_vec3Mul(PHE_VEC3* out, const PHE_VEC3* a, const PHE_VEC3* b)
{
    return Vec3Mul(out, a, b);
}
static inline PHE_VEC3* PHE_vec3Div(PHE_VEC3* out, const PHE_VEC3* a, const PHE_VEC3* b)
{
    return Vec3Div(out, a, b);
}

// Scalars.
static inline PHE_VEC3* PHE_vec3MulScalar(PHE_VEC3* out, const PHE_VEC3* a, scalar scl)
{
    return Vec3MulScalar(out, a, scl);
}
static inline PHE_VEC3* PHE_vec3DivScalar(PHE_VEC3* out, const PHE_VEC3* a, scalar scl)
{
    return Vec3DivScalar(out, a, scl);
}

#endif /* !PHE_USE_FIXED_POINT */

static inline PHE_VEC3* PHE_vec3Set(PHE_VEC3* out, scalar x, scalar y, scalar z)
{
    out->x = x;
    out->y = y;
    out->z = z;
    return out;
}
static inline PHE_VEC3* PHE_vec3Ident(PHE_VEC3* out, scalar ident)
{
    out->x = ident;
    out->y = ident;
    out->z = ident;
    return out;
}

#define PHE_vec3Zero(out) \
    PHE_vec3Ident(out, 0)






/*
   PHE general.
*/

static void InitPheContext(void)
{
    if (pheContext.initialized == TRUE)
    {
        return;
    }

    pheContext.arena = ArenaInit(MB(8), KB(8), ARENA_FLAG_GROWABLE);
    pheContext.initialized = TRUE;
}



static void PHE_zeroBody(PHE_BODY* body)
{
    _MEMSET(body, 0, sizeof(*body));
}

static PHE_NODE PHE_createNode(scalar x, scalar y, scalar z, scalar radius)
{
    PHE_NODE node;
    _MEMSET(&node, 0, sizeof(node));

    node.position.x = x;
    node.position.y = y;
    node.position.z = z;
    node.radius = radius;

    node.mass = 1;
    node.invMass = 1;

    return node;
}


extern PHE_BODY PHE_createSphere(scalar radius)
{
    PHE_BODY b;
    PHE_zeroBody(&b);

    b.type = BODY_SPHERE;

    b.nodeCount = 1;
    b.nodes[0] = PHE_createNode(0, 0, 0, radius);

    return b;
}

extern PHE_BODY PHE_createBox(PHE_VEC3 sizes)
{
    PHE_BODY b;
    PHE_zeroBody(&b);

    b.type = BODY_BOX;
    b.form = BODY_RIGID;

    b.collisionFlag = COLLISION_ALL;
    b.collisionMask = COLLISION_ALL;

    b.nodeCount = 8;
    // Top cap.
    b.nodes[0] = PHE_createNode(sizes.x, sizes.y, sizes.z, PHE_RADIUS_EPSILON);
    b.nodes[1] = PHE_createNode(sizes.x, sizes.y, -sizes.z, PHE_RADIUS_EPSILON);
    b.nodes[2] = PHE_createNode(-sizes.x, sizes.y, -sizes.z, PHE_RADIUS_EPSILON);
    b.nodes[3] = PHE_createNode(-sizes.x, sizes.y, sizes.z, PHE_RADIUS_EPSILON);

    // Bottom cap.
    b.nodes[4] = PHE_createNode(sizes.x, -sizes.y, sizes.z, PHE_RADIUS_EPSILON);
    b.nodes[5] = PHE_createNode(sizes.x, -sizes.y, -sizes.z, PHE_RADIUS_EPSILON);
    b.nodes[6] = PHE_createNode(-sizes.x, -sizes.y, -sizes.z, PHE_RADIUS_EPSILON);
    b.nodes[7] = PHE_createNode(-sizes.x, -sizes.y, sizes.z, PHE_RADIUS_EPSILON);

    return b;
}

extern PHE_BODY PHE_createCapsule(PHE_VEC3 a, PHE_VEC3 b, scalar aRadius, scalar bRadius)
{
    PHE_BODY bdy;
    PHE_zeroBody(&bdy);

    bdy.type = BODY_CAPSULE;
    bdy.form = BODY_RIGID;

    bdy.collisionFlag = COLLISION_ALL;
    bdy.collisionMask = COLLISION_ALL;

    bdy.nodeCount = 2;
    bdy.nodes[0] = PHE_createNode(a.x, a.y, a.z, aRadius);
    bdy.nodes[1] = PHE_createNode(b.x, b.y, b.z, bRadius);

    return bdy;
}

/*
   Physics body properties.
*/

extern PHE_VEC3 PHE_getMidpoint(PHE_BODY* body)
{
    // Simply calculates the arithmetic average of all the nodes.
    PHE_VEC3 sum;
    PHE_vec3Zero(&sum);
    if (body->nodeCount == 0)
    {
        return sum;
    }

    for (int i = 0; i < body->nodeCount; ++i)
    {
        PHE_NODE* node = &body->nodes[i];
        PHE_vec3Add(&sum, &sum, &node->position);
    }

    PHE_vec3DivScalar(&sum, &sum, body->nodeCount);

    return sum;
}

extern PHE_VEC3 PHE_getCenterOfGravity(PHE_BODY* body)
{
    // Calculates the weighted average of all the nodes.
    PHE_VEC3 sum;
    scalar quotient = 0;
    PHE_vec3Zero(&sum);

    for (int i = 0; i < body->nodeCount; ++i)
    {
        PHE_NODE* node = &body->nodes[i];

        PHE_VEC3 v;
        PHE_vec3MulScalar(&v, &node->position, node->mass);
        PHE_vec3Add(&sum, &sum, &v);

        quotient = PHE_scalarAdd(quotient, node->mass);
    }

    PHE_vec3DivScalar(&sum, &sum, quotient);

    return sum;

}



/*
   Physics world.
*/

extern PHE_WORLD* PHE_createWorld(void)
{
    if (!pheContext.initialized)
    {
        InitPheContext();
    }

    PHE_WORLD* world = (PHE_WORLD *) ArenaPush(pheContext.arena, sizeof(PHE_WORLD), 0);
    world->nObjects = 0;

    // Push buffer.
    size_t bytecount = sizeof(PHE_BODY) * PHE_WORLD_CAPACITY;
    world->bodies = (PHE_BODY *) ArenaPush(pheContext.arena, bytecount, 0);
    PoolInit(&world->objectPool, world->bodies, bytecount, sizeof(PHE_BODY));

    world->integrator = INTEGR_EULER_SEMI_IMPLICIT;
    world->gravityAccel = V3(0, -PHE_DEFAULT_GRAVITY, 0);

    world->initialized = TRUE;

    return world;
}


extern void PHE_worldTerminate(PHE_WORLD* world)
{
    PoolTerminate(&world->objectPool);
    world->nObjects = 0;
}

extern PHE_BODY* PHE_addBody(PHE_WORLD* world, PHE_BODY* body)
{
    if (world == NULL || body == NULL)
    {
        fprintf(stderr, "%s: Did you forget to initialize the world or body?\n", __func__);
        return NULL;
    }

    PHE_BODY* b = (PHE_BODY *) PoolAlloc(&world->objectPool);
    if (b != NULL)
    {
        _MEMCPY(b, body, sizeof(PHE_BODY));
        world->nObjects++;
    }

    return b;
}
extern void PHE_subBody(PHE_WORLD* world, int index)
{
    if (world == NULL || world->bodies == NULL)
    {
        return;
    }
    // ConSOYmer boundary checks.
    if (index < 0 || index >= world->nObjects)
    {
        return;
    }

    PoolFree(&world->objectPool, &world->bodies[index]);

    int lastIndex = world->nObjects - 1;
    if (index != lastIndex)
    {
        // List rearrangement (swaps with last element).
        world->bodies[index] = world->bodies[lastIndex];
    }
    world->nObjects--;
}

extern void PHE_worldClear(PHE_WORLD* world)
{
    size_t bytecount = sizeof(PHE_BODY) * world->nObjects;

    // 'Freeing' the pool simply removes the linked list connections.
    PoolFreeAll(&world->objectPool);
    _MEMSET(world->bodies, 0, bytecount);

    world->nObjects = 0;
    world->manifolds.count = 0;
}


/*
   Integrators.
*/

static void PHE_integrateEuler(PHE_WORLD* world, scalar dt);
static void PHE_integrateHeun(PHE_WORLD* world, scalar dt);

static void PHE_integrateEuler(PHE_WORLD* world, scalar dt)
{
    for (int i = 0; i < world->nObjects; ++i)
    {
        PHE_BODY* body = &world->bodies[i];

        for (int j = 0; j < body->nodeCount; ++j)
        {
            PHE_NODE* node = &body->nodes[j];

            PHE_VEC3 tmp;

            // Add force (F = ma, therefore a = F / m).
            PHE_vec3MulScalar(&node->accel, &node->forceAccum, node->invMass);

            // Set velocity.
            PHE_vec3MulScalar(&tmp, &node->accel, dt);
            PHE_vec3Add(&node->vel, &node->vel, &tmp);

            // Set position.
            PHE_vec3MulScalar(&tmp, &node->vel, dt);
            PHE_vec3Add(&node->position, &node->position, &tmp);
        }
    }
}

static void PHE_integrateHeun(PHE_WORLD* world, scalar dt)
{
    UNUSED(world);
    UNUSED(dt);
    fprintf(stderr, "%s: NOT IMPLEMENTED!\n", __func__);
}



/*
   Collision detection.
*/


static void ManifoldsInit(PHE_MANIFOLD_QUEUE* manifolds)
{
    if (manifolds->data == NULL)
    {
        manifolds->capacity = PHE_WORLD_MANIFOLD_CAPACITY;
        manifolds->count = 0;
        manifolds->data = (PHE_MANIFOLD *) _REALLOC(manifolds->data, sizeof(PHE_MANIFOLD) * manifolds->capacity);
        assert(manifolds->data != NULL && "OOM?");
    }
}



static void ManifoldsReserve(PHE_MANIFOLD_QUEUE* manifolds, int desired)
{
    if (manifolds->data == NULL)
    {
        manifolds->capacity = PHE_WORLD_MANIFOLD_CAPACITY;
        manifolds->count = 0;
    }
    if (desired >= manifolds->capacity)
    {
        size_t cap = manifolds->capacity;
        while (desired >= cap)
        {
            cap *= 2;
        }

        manifolds->data = (PHE_MANIFOLD *) _REALLOC(manifolds->data, sizeof(PHE_MANIFOLD) * cap);
        assert(manifolds->data != NULL && "OOM?");
    }
}

static void ManifoldsAppend(PHE_MANIFOLD_QUEUE* manifolds, PHE_MANIFOLD item)
{
    ManifoldsReserve(manifolds, manifolds->count + 1);
    manifolds->data[manifolds->count++] = item;
}


static void ManifoldsEmpty(PHE_MANIFOLD_QUEUE* manifolds)
{
    manifolds->count = 0;
}

static void ManifoldsFree(PHE_MANIFOLD_QUEUE* manifolds)
{
    if (manifolds->data != NULL)
    {
        _FREE(manifolds->data);
        manifolds->data = NULL;
    }
    manifolds->count = 0;
}




extern void PHE_tick(PHE_WORLD* world, scalar dt)
{
    // Manifold queue is cleared.
    ManifoldsEmpty(&world->manifolds);


    // Set forces to 0.
    for (int i = 0; i < world->nObjects; ++i)
    {
        // Set forces to 0.
        PHE_BODY* body = &world->bodies[i];
        PHE_vec3Zero(&body->forceAccum);

        // Then set the internal forces to 0.
        for (int j = 0; j < body->nodeCount; ++j)
        {
            PHE_vec3Zero(&body->nodes[j].forceAccum);
        }
    }

    // Add forces.
    for (int i = 0; i < world->nObjects; ++i)
    {
        PHE_BODY* body = &world->bodies[i];

        // G = mg.
        for (int j = 0; j < body->nodeCount; ++j)
        {
            PHE_NODE* node = &body->nodes[j];
            PHE_vec3MulScalar(&node->forceAccum, &world->gravityAccel, node->mass);
        }
    }

    // Integrate physics.
    switch (world->integrator)
    {
        case INTEGR_EULER_SEMI_IMPLICIT:
        {
            PHE_integrateEuler(world, dt);
        } break;

        case INTEGR_HEUN:
        {
            PHE_integrateHeun(world, dt);
        } break;
    }



    // Check collisions.
    for (int i = 0; i < world->nObjects; ++i)
    {
        for (int j = 0; j < world->nObjects; ++j)
        {
            if (i == j)
            {
                continue;
            }

            // Put things as a list of Manifolds.
        }
    }
}




extern void PHE_terminate(void)
{
    if (!pheContext.initialized)
    {
        return;
    }

    ArenaTerminate(pheContext.arena);
    pheContext.initialized = FALSE;
}
