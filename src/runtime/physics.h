#ifndef PHYSICS_H_
#define PHYSICS_H_ 1


#define PHE_USE_FIXED_POINT

#ifdef PHE_USE_FIXED_POINT
#include "../base/mathf_fixed.h"
#else
#include "../base/mathf.h"
#endif


#include "../base/base_defs.h"
#include "../misc/pool.h"
#include "../misc/arena.h"



#ifndef scalar
#ifdef PHE_USE_FIXED_POINT
#define scalar fix16_t
#else
#define scalar float
#endif
#endif /* scalar */


#define PHE_NODE_COUNT 8

#define COLLISION_FLAG_GROUND 1
#define COLLISION_FLAG_PLAYER 2
#define COLLISION_FLAG_ENEMY 4
#define COLLISION_FLAG_TRIGGER 8

#define COLLISION_ALL ((uint64_t) ~0)



typedef enum PHE_BodyTypes {
    BODY_NONE = 0,

    BODY_SPHERE,
    BODY_BOX,
    BODY_CAPSULE,

    BODY_N
} PHE_BodyTypes;

typedef enum PHE_BodyForms {
    BODY_RIGID,
    BODY_SOFT,
} PHY_BodyForms;

typedef enum PHE_IntegratorTypes {
    INTEGR_EULER_SEMI_IMPLICIT,
    INTEGR_HEUN,
} PHE_IntegratorTypes;

typedef enum PHE_ColliderTypes {
    COLLIDER_NONE = 0,

    COLLIDER_SPHERE,
    COLLIDER_BOX,
    COLLIDER_CAPSULE,

    COLLIDER_N,
} PHE_ColliderTypes;




#ifdef PHE_USE_FIXED_POINT
typedef struct PHE_VEC3 {
    scalar x, y, z;
} PHE_VEC3;
#else
typedef vec3 PHE_VEC3;
#endif

#define V3(x, y, z) CLITERAL(PHE_VEC3) { x, y, z }


typedef struct SPHERE_COLLIDER {
    PHE_VEC3 center;
    scalar radius;
} SPHERE_COLLIDER;

typedef struct BOX_COLLIDER {
    PHE_VEC3 min;
    PHE_VEC3 max;
} BOX_COLLIDER;


typedef struct PHE_COLLIDER {
    PHE_ColliderTypes type;
    union {
        SPHERE_COLLIDER sph;
        BOX_COLLIDER box;
    };
} PHE_COLLIDER;

typedef struct PHE_NODE {
    PHE_VEC3 position;
    PHE_VEC3 vel;
    PHE_VEC3 accel;

    PHE_VEC3 forceAccum;

    scalar mass;
    scalar invMass;
    scalar radius;
} PHE_Node;

typedef struct PHE_BODY {
    PHE_BodyTypes type;
    PHE_BodyForms form;

    // Can the flag can be affected by the collision mask?
    uint64_t collisionFlag;
    uint64_t collisionMask;

    PHE_VEC3 center;
    PHE_VEC3 vel;
    PHE_VEC3 accel;

    PHE_VEC3 forceAccum;

    PHE_COLLIDER collider;

    PHE_NODE nodes[PHE_NODE_COUNT];
    int nodeCount;
} PHE_BODY;

typedef struct PHE_MANIFOLD {
    PHE_BODY* A;
    PHE_BODY* B;

    PHE_VEC3 AtoB;
    PHE_VEC3 BtoA;
    PHE_VEC3 normal;
    scalar depth;
} PHE_MANIFOLD;

typedef struct PHE_MANIFOLD_QUEUE {
    PHE_MANIFOLD* data;
    size_t count;
    size_t capacity;
} PHE_MANIFOLD_QUEUE;



typedef struct PHE_WORLD {
    PHE_IntegratorTypes integrator;

    PHE_MANIFOLD_QUEUE manifolds;

    PHE_VEC3 gravityAccel;

    POOL objectPool;
    PHE_BODY* bodies;
    int nObjects;

    b32 initialized;
} PHE_WORLD;

typedef struct PHE_CONTEXT {
    // Use a memory arena.
    ARENA* arena;

    b32 initialized;
} PHE_CONTEXT;

extern PHE_CONTEXT pheContext;


extern PHE_BODY PHE_createSphere(scalar radius);
extern PHE_BODY PHE_createBox(PHE_VEC3 sizes);
extern PHE_BODY PHE_createCapsule(PHE_VEC3 a, PHE_VEC3 b, scalar aRadius, scalar bRadius);

/*
   Colliders.
*/

/*
   Physics body properties.
*/

extern PHE_VEC3 PHE_getMidpoint(PHE_BODY* body);
extern PHE_VEC3 PHE_getCenterOfGravity(PHE_BODY* body);


/*
   Physics world.
*/

extern PHE_WORLD* PHE_createWorld(void);
extern void PHE_worldTerminate(PHE_WORLD* world);

/*
   Physics bodies.
*/

extern PHE_BODY* PHE_addBody(PHE_WORLD* world, PHE_BODY* body);
extern void PHE_subBody(PHE_WORLD* world, int index);

// Removes all objects.
extern void PHE_worldClear(PHE_WORLD* world);



extern void PHE_terminate(void);




#endif /* PHYSICS_H_ */
