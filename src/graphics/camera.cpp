#include "camera.h"
#include "../platform/input.h"
#include "../platform/window.h"

#include <string.h>
#include <math.h>


#define CAMERA_SPEED (0.05f)
#define CAMERA_SENSITIVITY (0.1f)

static CAMERA3CONTROLLER controller;

extern int Camera3Init(CAMERA3D* cam)
{
    _MEMSET(cam, 0, sizeof(*cam));
    cam->pos = CLITERAL(vec3) { 0, 0, -1 };
    cam->front = CLITERAL(vec3) { 0, 0, 1 };
    cam->up = CLITERAL(vec3)  { 0, 1, 0 };

    // Default optical settings.
    cam->fovy = RADIANS(90.0f);
    // TODO: Set up frame width.
    //cam->aspect = (float) graphics.frameHeight / graphics.frameWidth;
    cam->aspect = (float)windowHandle->h / windowHandle->w;

    // Also set up the controller.
    _MEMSET(&controller, 0, sizeof(controller));
    controller.speed = CAMERA_SPEED;
    controller.sensitivity = CAMERA_SENSITIVITY;
    controller.yaw = -90.0f;
    controller.lastX = 400;
    controller.lastY = 300;

    return 0;
}

static int Camera3ProcessInput(CAMERA3D* cam, float dt)
{
    // ROTATION COMPONENT.
    // We calculate yaw pitch angles at first.
    controller.yaw += controller.xoffset;
    controller.pitch += controller.yoffset;
    // Clamp.
    if (controller.pitch > 89.0f)
    {
        controller.pitch = 89.0f;
    }
    if (controller.pitch < -89.0f)
    {
        controller.pitch = -89.0f;
    }

    vec3 direction;
    direction.x = cosf(RADIANS(controller.yaw)) * cosf(RADIANS(controller.pitch));
    direction.y = sinf(RADIANS(controller.pitch));
    direction.z = sinf(RADIANS(controller.yaw)) * cosf(RADIANS(controller.pitch));
    direction = direction.Nor();
    controller.dir = direction;

    // TRANSLATION COMPONENT.
    vec3 vel = { 0, 0, 0 };
    vec3 strafe = vec3::Cross(controller.dir, cam->up).Nor();

    if (IsKeyDown('W'))
    {
        vel = vel + controller.dir;// * CAMERA_SPEED;
    }
    if (IsKeyDown('S'))
    {
        vel = vel - controller.dir;// * CAMERA_SPEED;
    }
    if (IsKeyDown('A'))
    {
        vel = vel - strafe;// * CAMERA_SPEED;
    }
    if (IsKeyDown('D'))
    {
        vel = vel + strafe;// * CAMERA_SPEED;
    }
    vel = vel * controller.speed;

    controller.vel = vel;

    // Also the mouse.
    controller.xoffset = (input.mouseXPos - controller.lastX) * controller.sensitivity;
    controller.yoffset = (controller.lastY - input.mouseYPos) * controller.sensitivity;
    controller.lastX = input.mouseXPos;
    controller.lastY = input.mouseYPos;

    return 0;
}

extern int Camera3Step(CAMERA3D *cam, float dt)
{
    // Controlling the camera.
    Camera3ProcessInput(cam, dt);

    cam->pos = cam->pos + controller.vel * dt;
    cam->front = controller.dir;

    return 0;
}
