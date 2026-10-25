#ifndef WIN_EVENTS_H_
#define WIN_EVENTS_H_ 1

#include "base/base_defs.h"
#include "base/math/mathf.h"

typedef u8 Key_t;
typedef u8 MouseButton_t;

typedef struct EventMouseMove {
    vec2 offset;
} EventMouseMove;

typedef struct EventMouseUp {
    vec2 pos;
    MouseButton_t button;
} EventMouseUp;

typedef struct EventMouseDown {
    vec2 pos;
    MouseButton_t button;
} EventMouseDown;

typedef struct EventKeyUp {
    Key_t key;
} EventKeyUp;

typedef struct EventKeyDown {
    Key_t key;
} EventKeyDown;

typedef struct EventScroll {
    float deltaX;
} EventScroll;




typedef enum WindowEventType {
    WIN_EVENT_MOUSE_MOVE,
    WIN_EVENT_MOUSE_UP,
    WIN_EVENT_MOUSE_DOWN,
    WIN_EVENT_SCROLL,

    WIN_EVENT_KEY_UP,
    WIN_EVENT_KEY_DOWN,
} WindowEventType;

typedef struct WindowEvent {
    struct WindowEvent* next;

    WindowEventType type;

    union {
        EventMouseMove mouseMove;
        EventMouseUp mouseUp;
        EventMouseDown mouseDown;
        EventScroll scroll;

        EventKeyUp keyUp;
        EventKeyDown keyDown;
    };
} WindowEvent;



typedef enum MouseButtons {
    MB_MOUSE_LEFT,
    MB_MOUSE_MIDDLE,
    MB_MOUSE_RIGHT,
    MB_MOUSE_COUNT,
} MouseButtons;

typedef enum WinKeys {
    M_KEY_NONE = 0,
    M_KEY_BACKSPACE,
    M_KEY_TAB,
    M_KEY_ENTER,
    M_KEY_CAPSLOCK,
    M_KEY_ESCAPE,
    M_KEY_SPACE,
    M_KEY_PAGEUP,
    M_KEY_PAGEDOWN,
    M_KEY_END,
    M_KEY_HOME,
    M_KEY_ARROW_LEFT,
    M_KEY_ARROW_UP,
    M_KEY_ARROW_RIGHT,
    M_KEY_ARROW_DOWN,
    M_KEY_INSERT,
    M_KEY_DELETE,
    M_KEY_NUMPAD_0,
    M_KEY_NUMPAD_1,
    M_KEY_NUMPAD_2,
    M_KEY_NUMPAD_3,
    M_KEY_NUMPAD_4,
    M_KEY_NUMPAD_5,
    M_KEY_NUMPAD_6,
    M_KEY_NUMPAD_7,
    M_KEY_NUMPAD_8,
    M_KEY_NUMPAD_9,
    M_KEY_NUMPAD_MULTIPLY,
    M_KEY_NUMPAD_ADD,
    M_KEY_NUMPAD_SUBTRACT,
    M_KEY_NUMPAD_DECIMAL,
    M_KEY_NUMPAD_DIVIDE,
    M_KEY_NUMPAD_ENTER,
    M_KEY_F1,
    M_KEY_F2,
    M_KEY_F3,
    M_KEY_F4,
    M_KEY_F5,
    M_KEY_F6,
    M_KEY_F7,
    M_KEY_F8,
    M_KEY_F9,
    M_KEY_F10,
    M_KEY_F11,
    M_KEY_F12,
    M_KEY_NUM_LOCK,
    M_KEY_SCROLL_LOCK,
    M_KEY_SHIFT,
    M_KEY_CONTROL,
    M_KEY_ALT,
    M_KEY_0 = '0',
    M_KEY_1 = '1',
    M_KEY_2 = '2',
    M_KEY_3 = '3',
    M_KEY_4 = '4',
    M_KEY_5 = '5',
    M_KEY_6 = '6',
    M_KEY_7 = '7',
    M_KEY_8 = '8',
    M_KEY_9 = '9',
    M_KEY_SEMICOLON,
    M_KEY_EQUAL,
    M_KEY_A = 'A',
    M_KEY_B = 'B',
    M_KEY_C = 'C',
    M_KEY_D = 'D',
    M_KEY_E = 'E',
    M_KEY_F = 'F',
    M_KEY_G = 'G',
    M_KEY_H = 'H',
    M_KEY_I = 'I',
    M_KEY_J = 'J',
    M_KEY_K = 'K',
    M_KEY_L = 'L',
    M_KEY_M = 'M',
    M_KEY_N = 'N',
    M_KEY_O = 'O',
    M_KEY_P = 'P',
    M_KEY_Q = 'Q',
    M_KEY_R = 'R',
    M_KEY_S = 'S',
    M_KEY_T = 'T',
    M_KEY_U = 'U',
    M_KEY_V = 'V',
    M_KEY_W = 'W',
    M_KEY_X = 'X',
    M_KEY_Y = 'Y',
    M_KEY_Z = 'Z',


    M_KEY_COMMA,
    M_KEY_PERIOD,
    M_KEY_MINUS,
    M_KEY_FORWARDSLASH,
    M_KEY_BACKSLASH,
    M_KEY_BACKTICK,
    M_KEY_LBRACKET,
    M_KEY_RBRACKET,
    M_KEY_APOSTROPHE,

    M_KEY_COUNT,
} WinKeys;

#endif /* WIN_EVENTS_H_ */
