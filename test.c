#include "windowing/windowing.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>
#include <string.h>

const int initial_width  = 800;
const int initial_height = 600;

struct Window_State {
    bool destroyed;
    bool should_close;
    int width, height;
};

struct Window {
    void *handle;
    struct Window_State *state;
};

inline static struct Window_State *window_get_state_from_handle(void *handle) {
    LONG_PTR ptr = GetWindowLongPtrA(handle, GWLP_USERDATA);
    struct Window_State *state = (struct Window_State *) ptr;
    return state;
}

static LRESULT CALLBACK window_proc(
    HWND hwnd, UINT msg,
    WPARAM wParam, LPARAM lParam
) {
    struct Window_State *state = NULL;

    // @Ref: https://learn.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
    // @Ref: https://stackoverflow.com/questions/4270673/c-c-windows-how-to-keep-track-of-hwnd-context-data
    if (msg == WM_CREATE) {
        CREATESTRUCT *pCreate = (CREATESTRUCT *) lParam;
        state = (struct Window_State *) pCreate->lpCreateParams;
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR) state);

    } else {
        state = window_get_state_from_handle(hwnd);
    }

    LRESULT result = 0;
    switch (msg) {

    case WM_DESTROY: {
        state->should_close = true;
        PostQuitMessage(0);
    } break;

    case WM_SIZE: {
        RECT client_rect;
        GetClientRect(hwnd, &client_rect);
        state->width  = client_rect.right  - client_rect.left;
        state->height = client_rect.bottom - client_rect.top;
    } break;

    default: {
        result = DefWindowProcA(hwnd, msg, wParam, lParam);
    } break;
    }

    return result;
}

struct Window window_create(int width, int height, const char *title) {
    HINSTANCE current_process_handle = GetModuleHandleA(NULL);

    WNDCLASSA class = { 0 };
    class.lpszClassName = "Some Window Class";
    class.hInstance     = current_process_handle;
    class.lpfnWndProc   = window_proc;
    RegisterClassA(&class);

    static const DWORD window_style = WS_OVERLAPPEDWINDOW;

    RECT border_rect = { 0 };
    AdjustWindowRectEx(&border_rect, window_style, 0, 0);
    width += border_rect.right - border_rect.left;
    height += border_rect.bottom - border_rect.top;

    struct Window w = { 0 };

    w.state = calloc(1, sizeof(struct Window_State)); // @libc
    memset(w.state, 0, sizeof(struct Window_State));  // @libc
    w.state->width  = width;
    w.state->height = height;

    w.handle = CreateWindowExA(
        0, class.lpszClassName,
        title,
        window_style,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, current_process_handle, w.state
    );

    // https://stackoverflow.com/questions/11118443/why-we-need-to-call-updatewindow-following-showwindow#11119645
    ShowWindow(w.handle, SW_SHOW);
    UpdateWindow(w.handle);
    return w;
}

void window_destroy(struct Window *window) {
    if (window->handle) {
        DestroyWindow(window->handle);
        free(window->state); // @libc
        window->handle = NULL;
    }
}

bool window_should_close(struct Window window) {
    return window.state->should_close;
}

int main(void) {
    struct Window window = window_create(initial_width, initial_height, "Window Title");

    while (!window_should_close(window)) {
        bool result = 0;
        MSG msg = { 0 };
        if (PeekMessageA(&msg, window.handle, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    window_destroy(&window);
    return 0;
}
