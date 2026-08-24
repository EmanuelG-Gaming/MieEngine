#include "input.h"
#include "../base/base_defs.h"
#include <string.h>

INPUTHANDLE input;


extern int InputInit(void)
{
    _MEMSET(&input, 0, sizeof(input));

    input.mouseXPos = 0;
    input.mouseYPos = 0;

    // Initialize all the keys.
    for (int i = 0; i < 256; ++i)
    {
        input.keys[i] = FALSE;
    }

    return 0;
}
extern void InputTerminate(void)
{
    input.mouseXPos = 0;
    input.mouseYPos = 0;
}


extern int DoKeyDown(unsigned int key)
{
    input.keys[key] = TRUE;
    return 0;
}
extern int DoKeyUp(unsigned int key)
{
    input.keys[key] = FALSE;
    return 0;
}



extern int IsKeyDown(unsigned int key)
{
    return input.keys[key];
}
extern int IsKeyUp(unsigned int key)
{
    return !input.keys[key];
}


extern int SetMouseXPosition(float x)
{
    input.mouseXPos = x;
    return 0;
}
extern int SetMouseYPosition(float y)
{
    input.mouseYPos = y;
    return 0;
}

extern float GetMouseXPosition(void)
{
    return input.mouseXPos;
}
extern float GetMouseYPosition(void)
{
    return input.mouseYPos;
}
