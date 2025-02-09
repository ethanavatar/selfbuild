#if defined(_WIN32)
#include "windowing/windowing.h"

#define SOGL_IMPL
#include "sogl.h"

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PIXELFORMATDESCRIPTOR window_opengl_set_pixel_format(
    HINSTANCE instance, WNDCLASSA class
);

HGLRC window_opengl_set_rendering_context(
    HWND window_handle, HDC device_context, PIXELFORMATDESCRIPTOR pixel_format_descriptor
);

inline static struct Window_State *window_get_state_from_handle(void *handle) {
    LONG_PTR ptr = GetWindowLongPtrA(handle, GWLP_USERDATA);
    struct Window_State *state = (struct Window_State *) ptr;
    return state;
}

static LRESULT CALLBACK window_proc(
    HWND hwnd, UINT msg,
    WPARAM wParam, LPARAM lParam
);

struct Window window_create(struct Window_Description description) {
    HINSTANCE current_process_handle = GetModuleHandleA(NULL);

    WNDCLASSA class = { 0 };
    class.lpszClassName = "Some Window Class";
    class.hInstance     = current_process_handle;
    class.lpfnWndProc   = window_proc;
    RegisterClassA(&class);

    RECT border_rect = { 0 };
    AdjustWindowRectEx(&border_rect, WS_OVERLAPPEDWINDOW, 0, 0);
    int width  = description.width  + border_rect.right  - border_rect.left;
    int height = description.height + border_rect.bottom - border_rect.top;

    struct Window w = { 0 };
    w.state = calloc(1, sizeof(struct Window_State)); // @libc
    memset(w.state, 0, sizeof(struct Window_State));  // @libc
    w.state->width   = width;
    w.state->height  = height;
    w.state->backend = description.backend;

    if (description.backend.kind == Graphics_Backend_OpenGL) {
        w.state->pixel_format_descriptor = window_opengl_set_pixel_format(
            current_process_handle, class
        );
    }

    w.handle = CreateWindowExA(
        0, class.lpszClassName, description.title,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, current_process_handle,
        w.state
    );

    // https://stackoverflow.com/questions/11118443/why-we-need-to-call-updatewindow-following-showwindow#11119645
    ShowWindow(w.handle, SW_SHOW);
    UpdateWindow(w.handle);
    return w;
}

void window_destroy(struct Window *window) {
    if (window->handle) {
        DestroyWindow(window->handle);
        window->handle = NULL;
    }

    if (window->state) {
        free(window->state); // @libc
    }
}

bool window_should_close(struct Window window) {
    return window.state->should_close;
}

static thread_local struct Window *currently_drawing_window = NULL;

void window_draw_begin(struct Window *window) {
    struct Window_State *state = window->state;
    wglMakeCurrent(state->device_context, state->rendering_context);
    currently_drawing_window = window;
}

void window_draw_end(void) {
    struct Window *window = currently_drawing_window;
    struct Window_State *state = window->state;

    MSG msg = { 0 };
    if (PeekMessageA(&msg, window->handle, 0, 0, PM_REMOVE)) {
        bool should_process = msg.message != WM_QUIT;
        if (should_process) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    SwapBuffers(state->device_context);
    currently_drawing_window = NULL;

    wglMakeCurrent(NULL, NULL);
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

        state->device_context = GetDC(hwnd);

        if (state->backend.kind == Graphics_Backend_OpenGL) {
            state->rendering_context = window_opengl_set_rendering_context(
                hwnd, state->device_context, state->pixel_format_descriptor
            );
        }

    } else {
        state = window_get_state_from_handle(hwnd);
    }

    LRESULT result = 0;
    switch (msg) {

    //case WM_KEYUP:
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        unsigned int vk_code = wParam;
        if (0) { }
        else if (vk_code == VK_ESCAPE) { state->should_close = true; }
    } break;

    case WM_DESTROY: {
        state->should_close = true;

        if (state->backend.kind == Graphics_Backend_OpenGL) {
            wglMakeCurrent(NULL, NULL);
            ReleaseDC(hwnd, state->device_context);
            wglDeleteContext(state->rendering_context);
        }

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

#endif
