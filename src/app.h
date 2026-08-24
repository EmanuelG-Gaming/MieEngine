#ifndef APP_H_
#define APP_H_ 1

#include "graphics/gfx.h"
#include "platform/window.h"

extern int AppInit(WINDOW* win);
extern void AppShutdown(void);

extern int AppFrame(void);
extern int AppRender(void);


#endif /* APP_H_ */
