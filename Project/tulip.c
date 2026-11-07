
/*
 *   Source [tulip.c]
 */

#include "tulip.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

#define TULIP_WINDOW_STYLE \
(WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX))

static HINSTANCE g_instance = NULL;
static HWND      g_hwnd     = NULL;
static bool      g_running  = false;
static uInt32    g_screen_width  = 0u;
static uInt32    g_screen_height = 0u;
static uInt32    g_screen_scale  = 1u;
static uInt32*   g_screen_buffer = NULL;

static TULIP_Mouse g_mouse = { 0 };
static bool g_mouse_primed = false;
static bool g_mouse_curr[TULIP_MOUSE_COUNT] = { 0 };
static bool g_mouse_prev[TULIP_MOUSE_COUNT] = { 0 };

static bool g_key_curr[TULIP_KEY_COUNT] = { 0 };
static bool g_key_prev[TULIP_KEY_COUNT] = { 0 };

static void present_to(HDC);
static void mouse_button(HWND, TULIP_MouseButton, bool down);
static TULIP_Key scancode_to_key(LPARAM);
static LRESULT CALLBACK window_proc(HWND, UINT, WPARAM, LPARAM);

/*
 *   Window API
 */

void TULIP_WindowInit(const char* title, uInt32 width, uInt32 height, uInt32 scale)
{
	assert(width > 0 && height > 0);
	assert(scale > 0);

	g_screen_width  = width;
	g_screen_height = height;
	g_screen_scale  = scale;
	g_screen_buffer = calloc((size_t)width * height, sizeof(uInt32));

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	HMODULE instance = GetModuleHandleW(NULL);

	WNDCLASSEXW wc = {
		.cbSize        = sizeof(WNDCLASSEXW),
		.lpfnWndProc   = window_proc,
		.hInstance     = instance,
		.hCursor       = LoadCursorW(NULL, IDC_ARROW),
		.hIcon         = LoadIconW(NULL, IDI_APPLICATION),
		.hbrBackground = NULL,
		.lpszClassName = L"MainWindowClass",
	};

	RegisterClassExW(&wc);

	Int32 len = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
	wchar_t* wtitle = calloc(len, sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, len);

	RECT rect = { 0, 0, (LONG)(width * scale), (LONG)(height * scale) };
	AdjustWindowRectExForDpi(&rect, TULIP_WINDOW_STYLE, FALSE, 0, GetDpiForSystem());

	DWORD dwExStyle = 0;
	HWND hwnd = CreateWindowExW(
		dwExStyle,
		L"MainWindowClass",
		wtitle,
		TULIP_WINDOW_STYLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		NULL,
		NULL,
		instance,
		NULL
	);

	free(wtitle);
	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	g_instance = instance;
	g_hwnd     = hwnd;
	g_running  = true;
}

void TULIP_WindowQuit(void)
{
	if (g_hwnd) DestroyWindow(g_hwnd);
	UnregisterClassW(L"MainWindowClass", g_instance);
	
	g_instance = NULL;
	g_hwnd     = NULL;
	g_running  = false;

	free(g_screen_buffer);
	g_screen_buffer = NULL;
	g_screen_width  = 0;
	g_screen_height = 0;
	g_screen_scale  = 1;

	g_mouse_primed = false;
	memset(&g_mouse,     0, sizeof(g_mouse));
	memset(g_mouse_curr, 0, sizeof(g_mouse_curr));
	memset(g_mouse_prev, 0, sizeof(g_mouse_prev));
	memset(g_key_curr,   0, sizeof(g_key_curr));
	memset(g_key_prev,   0, sizeof(g_key_prev));
}

bool TULIP_WindowIsOpen(void)
{
	g_mouse.xvel   = 0.0f;
	g_mouse.yvel   = 0.0f;
	g_mouse.scroll = 0.0f;

	memcpy(g_mouse_prev, g_mouse_curr, sizeof(bool) * TULIP_MOUSE_COUNT);
	memcpy(g_key_prev, g_key_curr, sizeof(bool) * TULIP_KEY_COUNT);

	MSG msg;
	while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			g_running = false;
			break;
		}

		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return g_running;
}

uInt32 TULIP_WindowWidth(void)
{
	return g_screen_width;
}

uInt32 TULIP_WindowHeight(void)
{
	return g_screen_height;
}

uInt32* TULIP_RenderBuffer(void)
{
	return g_screen_buffer;
}

/*
 *   Render API
 */

void TULIP_RenderClear(uInt32 color)
{
	uInt32 count = g_screen_width * g_screen_height;

	for (uInt32 i = 0; i < count; i++) {
		g_screen_buffer[i] = color;
	}
}

void TULIP_RenderClearFast(void)
{
	uInt32 count = g_screen_width * g_screen_height;
	memset(g_screen_buffer, 0, count * sizeof(uInt32));
}

void TULIP_RenderPixel(uInt32 xpos, uInt32 ypos, uInt32 color)
{
	assert(xpos < g_screen_width);
	assert(ypos < g_screen_height);
	uInt32 index = ypos * g_screen_width + xpos;
	g_screen_buffer[index] = color;
}

void TULIP_RenderPixelSafe(Int32 xpos, Int32 ypos, uInt32 color)
{
	const bool bad_bounds =
		((uInt32)xpos >= g_screen_width) ||
		((uInt32)ypos >= g_screen_height);
	
	if (bad_bounds)
		return;
	
	uInt32 index = (uInt32)ypos * g_screen_width + (uInt32)xpos;
	g_screen_buffer[index] = color;
}

void TULIP_RenderFrame(void)
{
	if (!g_hwnd)
		return;

	HDC hdc = GetDC(g_hwnd);
	present_to(hdc);
	ReleaseDC(g_hwnd, hdc);
}

/*
 *   Mouse API
 */

TULIP_Mouse TULIP_MouseInput(void)
{
	return g_mouse;
}

bool TULIP_MousePressing(TULIP_MouseButton button)
{
	assert(button < TULIP_MOUSE_COUNT);
	return g_mouse_curr[button];
}

bool TULIP_MousePressed(TULIP_MouseButton button)
{
	assert(button < TULIP_MOUSE_COUNT);
	return g_mouse_curr[button] && !g_mouse_prev[button];
}

bool TULIP_MouseReleased(TULIP_MouseButton button)
{
	assert(button < TULIP_MOUSE_COUNT);
	return !g_mouse_curr[button] && g_mouse_prev[button];
}

/*
 *   Keyboard API
 */

bool TULIP_KeyboardPressing(TULIP_Key key)
{
	assert(key < TULIP_KEY_COUNT);
	return g_key_curr[key];
}

bool TULIP_KeyboardPressed(TULIP_Key key)
{
	assert(key < TULIP_KEY_COUNT);
	return g_key_curr[key] && !g_key_prev[key];
}

bool TULIP_KeyboardReleased(TULIP_Key key)
{
	assert(key < TULIP_KEY_COUNT);
	return !g_key_curr[key] && g_key_prev[key];
}

/*
 *   Internal
 */

static void present_to(HDC hdc)
{
	if (!g_screen_buffer)
		return;

	BITMAPINFO bmi = {
		.bmiHeader = {
			.biSize        = sizeof(BITMAPINFOHEADER),
			.biWidth       = (LONG)g_screen_width,
			.biHeight      = -(LONG)g_screen_height,
			.biPlanes      = 1,
			.biBitCount    = 32,
			.biCompression = BI_RGB,
		},
	};

	SetStretchBltMode(hdc, COLORONCOLOR);

	StretchDIBits(
		hdc,
		0, 0,
		(int)(g_screen_width  * g_screen_scale),
		(int)(g_screen_height * g_screen_scale),
		0, 0,
		(int)g_screen_width,
		(int)g_screen_height,
		g_screen_buffer,
		&bmi,
		DIB_RGB_COLORS,
		SRCCOPY
	);
}

static void mouse_button(HWND hwnd, TULIP_MouseButton button, bool down)
{
	g_mouse_curr[button] = down;

	bool any_down = false;
	for (uInt32 i = 0; i < TULIP_MOUSE_COUNT; i++) {
		bool pressed = g_mouse_curr[i];
		if (pressed) {
			any_down = true;
			break;
		}
	}

	if (any_down) {
		SetCapture(hwnd);
		return;
	}

	ReleaseCapture();
}

static TULIP_Key scancode_to_key(LPARAM lParam)
{
	uInt32 scancode = (lParam >> 16) & 0xFF;
	bool   extended = (lParam >> 24) & 0x01;

	if (extended) {
		switch (scancode) {
			case 0x1C: return TULIP_KEY_ENTER;
			case 0x1D: return TULIP_KEY_RCTRL;
			case 0x38: return TULIP_KEY_RALT;
			case 0x48: return TULIP_KEY_UP;
			case 0x50: return TULIP_KEY_DOWN;
			case 0x4B: return TULIP_KEY_LEFT;
			case 0x4D: return TULIP_KEY_RIGHT;
			default: return TULIP_KEY_COUNT;
		}
	}

	switch (scancode) {
		case 0x0B: return TULIP_KEY_0;
		case 0x02: return TULIP_KEY_1;
		case 0x03: return TULIP_KEY_2;
		case 0x04: return TULIP_KEY_3;
		case 0x05: return TULIP_KEY_4;
		case 0x06: return TULIP_KEY_5;
		case 0x07: return TULIP_KEY_6;
		case 0x08: return TULIP_KEY_7;
		case 0x09: return TULIP_KEY_8;
		case 0x0A: return TULIP_KEY_9;
		case 0x1E: return TULIP_KEY_A;
		case 0x30: return TULIP_KEY_B;
		case 0x2E: return TULIP_KEY_C;
		case 0x20: return TULIP_KEY_D;
		case 0x12: return TULIP_KEY_E;
		case 0x21: return TULIP_KEY_F;
		case 0x22: return TULIP_KEY_G;
		case 0x23: return TULIP_KEY_H;
		case 0x17: return TULIP_KEY_I;
		case 0x24: return TULIP_KEY_J;
		case 0x25: return TULIP_KEY_K;
		case 0x26: return TULIP_KEY_L;
		case 0x32: return TULIP_KEY_M;
		case 0x31: return TULIP_KEY_N;
		case 0x18: return TULIP_KEY_O;
		case 0x19: return TULIP_KEY_P;
		case 0x10: return TULIP_KEY_Q;
		case 0x13: return TULIP_KEY_R;
		case 0x1F: return TULIP_KEY_S;
		case 0x14: return TULIP_KEY_T;
		case 0x16: return TULIP_KEY_U;
		case 0x2F: return TULIP_KEY_V;
		case 0x11: return TULIP_KEY_W;
		case 0x2D: return TULIP_KEY_X;
		case 0x15: return TULIP_KEY_Y;
		case 0x2C: return TULIP_KEY_Z;
		case 0x39: return TULIP_KEY_SPACE;
		case 0x1C: return TULIP_KEY_ENTER;
		case 0x01: return TULIP_KEY_ESCAPE;
		case 0x0E: return TULIP_KEY_BACKSPACE;
		case 0x0F: return TULIP_KEY_TAB;
		case 0x2A: return TULIP_KEY_LSHIFT;
		case 0x36: return TULIP_KEY_RSHIFT;
		case 0x1D: return TULIP_KEY_LCTRL;
		case 0x38: return TULIP_KEY_LALT;
		case 0x3B: return TULIP_KEY_F1;
		case 0x3C: return TULIP_KEY_F2;
		case 0x3D: return TULIP_KEY_F3;
		case 0x3E: return TULIP_KEY_F4;
		case 0x3F: return TULIP_KEY_F5;
		case 0x40: return TULIP_KEY_F6;
		case 0x41: return TULIP_KEY_F7;
		case 0x42: return TULIP_KEY_F8;
		case 0x43: return TULIP_KEY_F9;
		case 0x44: return TULIP_KEY_F10;
		case 0x57: return TULIP_KEY_F11;
		case 0x58: return TULIP_KEY_F12;
		default: return TULIP_KEY_COUNT;
	}
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_DESTROY) {
		g_hwnd = NULL;
		g_running = false;
		return 0;
	}

	if (msg == WM_MOUSEMOVE) {
		float x = (float)GET_X_LPARAM(lParam) / (float)g_screen_scale;
		float y = (float)GET_Y_LPARAM(lParam) / (float)g_screen_scale;

		if (g_mouse_primed) {
			g_mouse.xvel += x - g_mouse.xpos;
			g_mouse.yvel += y - g_mouse.ypos;
		}

		g_mouse.xpos = x;
		g_mouse.ypos = y;
		g_mouse_primed = true;
		return 0;
	}

	if (msg == WM_MOUSEWHEEL) {
		g_mouse.scroll +=
			(float)GET_WHEEL_DELTA_WPARAM(wParam) /
			(float)WHEEL_DELTA;
		return 0;
	}

	if (msg == WM_LBUTTONDOWN) { mouse_button(hwnd, TULIP_MOUSE_LEFT,   true);  return 0; }
	if (msg == WM_LBUTTONUP)   { mouse_button(hwnd, TULIP_MOUSE_LEFT,   false); return 0; }
	if (msg == WM_RBUTTONDOWN) { mouse_button(hwnd, TULIP_MOUSE_RIGHT,  true);  return 0; }
	if (msg == WM_RBUTTONUP)   { mouse_button(hwnd, TULIP_MOUSE_RIGHT,  false); return 0; }
	if (msg == WM_MBUTTONDOWN) { mouse_button(hwnd, TULIP_MOUSE_MIDDLE, true);  return 0; }
	if (msg == WM_MBUTTONUP)   { mouse_button(hwnd, TULIP_MOUSE_MIDDLE, false); return 0; }

	if ((msg == WM_XBUTTONDOWN) ||
		(msg == WM_XBUTTONUP)
	) {
		TULIP_MouseButton button =
			(GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ?
			TULIP_MOUSE_X1 :
			TULIP_MOUSE_X2;

		const bool down = (msg == WM_XBUTTONDOWN);
		mouse_button(hwnd, button, down);
		return 1;
	}

	if ((msg == WM_KEYDOWN)    ||
		(msg == WM_KEYUP)      ||
		(msg == WM_SYSKEYDOWN) ||
		(msg == WM_SYSKEYUP)
	) {
		TULIP_Key key = scancode_to_key(lParam);
		if (key != TULIP_KEY_COUNT) {
			g_key_curr[key] = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
		}

		if ((msg == WM_SYSKEYDOWN) ||
			(msg == WM_SYSKEYUP)
		) {
			return DefWindowProcW(hwnd, msg, wParam, lParam);
		} return 0;
	}

	if (msg == WM_KILLFOCUS) {
		ReleaseCapture();
		for (uInt32 i = 0; i < TULIP_MOUSE_COUNT; i++) {
			g_mouse_curr[i] = false;
		}
		for (uInt32 i = 0; i < TULIP_KEY_COUNT; i++) {
			g_key_curr[i] = false;
		} return 0;
	}

	if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		present_to(hdc);
		EndPaint(hwnd, &ps);
		return 0;
	}

	if (msg == WM_DPICHANGED) {
		UINT  dpi       = HIWORD(wParam);
		RECT* suggested = (RECT*)lParam;
		RECT  rect      = {
			0, 0,
			(LONG)(g_screen_width  * g_screen_scale),
			(LONG)(g_screen_height * g_screen_scale),
		};
		AdjustWindowRectExForDpi(&rect, TULIP_WINDOW_STYLE, FALSE, 0, dpi);

		SetWindowPos(
			hwnd,
			NULL,
			suggested->left,
			suggested->top,
			rect.right - rect.left,
			rect.bottom - rect.top,
			SWP_NOZORDER | SWP_NOACTIVATE
		); return 0;
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

