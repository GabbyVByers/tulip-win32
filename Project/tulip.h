
/*
 *   Header [tulip.h]
 */

#ifndef TULIP_HEADER_GUARD
#define TULIP_HEADER_GUARD

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef int8_t  Int8;
typedef int16_t Int16;
typedef int32_t Int32;
typedef int64_t Int64;

typedef uint8_t  uInt8;
typedef uint16_t uInt16;
typedef uint32_t uInt32;
typedef uint64_t uInt64;

#define TULIP_COLOR(R, G, B) \
((uInt32)                    \
((uInt32)(R) << 16) |        \
((uInt32)(G) << 8 ) |        \
((uInt32)(B) << 0 ))

#define TULIP_COLOR_WHITE  TULIP_COLOR(255, 255, 255)
#define TULIP_COLOR_BLACK  TULIP_COLOR(  0,   0,   0)
#define TULIP_COLOR_RED    TULIP_COLOR(255,   0,   0)
#define TULIP_COLOR_GREEN  TULIP_COLOR(  0, 255,   0)
#define TULIP_COLOR_BLUE   TULIP_COLOR(  0,   0, 255)
#define TULIP_COLOR_CYAN   TULIP_COLOR(  0, 255, 255)
#define TULIP_COLOR_PURPLE TULIP_COLOR(255,   0, 255)
#define TULIP_COLOR_YELLOW TULIP_COLOR(255, 255,   0)

typedef struct TULIP_Mouse {
	float scroll;
	float xpos, ypos;
	float xvel, yvel;
} TULIP_Mouse;

typedef enum TULIP_MouseButton {
	TULIP_MOUSE_LEFT,
	TULIP_MOUSE_RIGHT,
	TULIP_MOUSE_MIDDLE,
	TULIP_MOUSE_X1,
	TULIP_MOUSE_X2,
	TULIP_MOUSE_COUNT,
} TULIP_MouseButton;

typedef enum TULIP_Key {
	TULIP_KEY_0,
	TULIP_KEY_1,
	TULIP_KEY_2,
	TULIP_KEY_3,
	TULIP_KEY_4,
	TULIP_KEY_5,
	TULIP_KEY_6,
	TULIP_KEY_7,
	TULIP_KEY_8,
	TULIP_KEY_9,
	TULIP_KEY_A,
	TULIP_KEY_B,
	TULIP_KEY_C,
	TULIP_KEY_D,
	TULIP_KEY_E,
	TULIP_KEY_F,
	TULIP_KEY_G,
	TULIP_KEY_H,
	TULIP_KEY_I,
	TULIP_KEY_J,
	TULIP_KEY_K,
	TULIP_KEY_L,
	TULIP_KEY_M,
	TULIP_KEY_N,
	TULIP_KEY_O,
	TULIP_KEY_P,
	TULIP_KEY_Q,
	TULIP_KEY_R,
	TULIP_KEY_S,
	TULIP_KEY_T,
	TULIP_KEY_U,
	TULIP_KEY_V,
	TULIP_KEY_W,
	TULIP_KEY_X,
	TULIP_KEY_Y,
	TULIP_KEY_Z,
	TULIP_KEY_SPACE,
	TULIP_KEY_ENTER,
	TULIP_KEY_ESCAPE,
	TULIP_KEY_BACKSPACE,
	TULIP_KEY_TAB,
	TULIP_KEY_UP,
	TULIP_KEY_DOWN,
	TULIP_KEY_LEFT,
	TULIP_KEY_RIGHT,
	TULIP_KEY_LSHIFT,
	TULIP_KEY_LCTRL,
	TULIP_KEY_LALT,
	TULIP_KEY_RSHIFT,
	TULIP_KEY_RCTRL,
	TULIP_KEY_RALT,
	TULIP_KEY_F1,
	TULIP_KEY_F2,
	TULIP_KEY_F3,
	TULIP_KEY_F4,
	TULIP_KEY_F5,
	TULIP_KEY_F6,
	TULIP_KEY_F7,
	TULIP_KEY_F8,
	TULIP_KEY_F9,
	TULIP_KEY_F10,
	TULIP_KEY_F11,
	TULIP_KEY_F12,
	TULIP_KEY_COUNT,
} TULIP_Key;

void        TULIP_WindowInit(const char* title, uInt32 width, uInt32 height, uInt32 scale);
void        TULIP_WindowQuit(void);
bool        TULIP_WindowIsOpen(void);
uInt32      TULIP_WindowWidth(void);
uInt32      TULIP_WindowHeight(void);

uInt32*     TULIP_RenderBuffer(void);
void        TULIP_RenderClear(uInt32 color);
void        TULIP_RenderClearFast(void);
void        TULIP_RenderPixel(uInt32 xpos, uInt32 ypos, uInt32 color);
void        TULIP_RenderPixelSafe(Int32 xpos, Int32 ypos, uInt32 color);
void        TULIP_RenderFrame(void);

TULIP_Mouse TULIP_MouseInput(void);
bool        TULIP_MousePressing(TULIP_MouseButton button);
bool        TULIP_MousePressed(TULIP_MouseButton button);
bool        TULIP_MouseReleased(TULIP_MouseButton button);

bool        TULIP_KeyboardPressing(TULIP_Key key);
bool        TULIP_KeyboardPressed(TULIP_Key key);
bool        TULIP_KeyboardReleased(TULIP_Key key);

#endif /* TULIP_HEADER_GUARD */

