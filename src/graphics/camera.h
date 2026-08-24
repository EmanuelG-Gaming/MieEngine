#ifndef CAMERA_H_
#define CAMERA_H_ 1

// Different camera modes.
#include "../base/mathf.h"

typedef struct CAMERA3D {
    vec3 pos;
    vec3 front;
    vec3 up;

    float fovy;
    float aspect;
} CAMERA3D;

typedef struct CAMERA3CONTROLLER {
    vec3 dir;
    vec3 vel;

    float yaw;
    float pitch;
    float roll;

    float speed;
    float sensitivity;

    // Mouse-related.
    float lastX, lastY;
    float xoffset, yoffset;
} CAMERA3CONTROLLER;

extern int Camera3Init(CAMERA3D* cam);
extern int Camera3Step(CAMERA3D* cam, float dt);

#endif /* CAMERA_H_ */
