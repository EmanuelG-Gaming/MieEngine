#ifndef GFX_H_
#define GFX_H_

// Ok so platform.h is the one responsible for win32.
//#include "../platform/platform.h"
//#include "../platform/window.h"
//extern CAMERA3D camera3D;

extern int GraphicsDriverInit(const wchar_t* title, int width, int height);
extern void GraphicsTerminate(void);

extern int GraphicsRun(void);


#endif /* GFX_H_ */
