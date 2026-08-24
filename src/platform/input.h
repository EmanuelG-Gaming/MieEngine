#ifndef INPUT_H_
#define INPUT_H_ 1

// Lightweight layer around input.

#define MOUSE_LEFT (0)
#define MOUSE_MIDDLE (1)
#define MOUSE_RIGHT (2)
#define MOUSE_MAX (3)

typedef struct INPUTHANDLE {
    float mouseXPos;
    float mouseYPos;

    float mouseScrollX;
    float mouseScrollY;
    
    int keys[256];
    int mouseButtons[MOUSE_MAX];
} INPUTHANDLE;

extern INPUTHANDLE input;

extern int InputInit(void);
extern void InputTerminate(void);

extern int DoKeyDown(unsigned int key);
extern int DoKeyUp(unsigned int key);


extern int IsKeyDown(unsigned int key);

extern int SetMouseXPosition(float x);
extern int SetMouseYPosition(float y);
extern float GetMouseXPosition(void);
extern float GetMouseYPosition(void);


#endif /* INPUT_H_ */
