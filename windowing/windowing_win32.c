#if defined(_WIN32)
#include "windowing/windowing.h"
#include "windowing/windowing_win32_private.h"

#define SOGL_IMPL
#include "sogl.h"
// Needs to be included after sogl
#include "windowing/windowing_win32_opengl.h"

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Window window_create(struct Window_Description description) {
    HINSTANCE current_process_handle = GetModuleHandleA(NULL);

    WNDCLASSA class = { 0 };
    class.lpszClassName = "Some Window Class";
    class.hInstance     = current_process_handle;
    class.lpfnWndProc   = window_proc;
    RegisterClassA(&class);

    PIXELFORMATDESCRIPTOR opengl_pixel_format;
    if (description.backend == Graphics_Backend_OpenGL) {
        opengl_pixel_format = window_opengl_set_pixel_format(
            current_process_handle, class
        );
    }

    static const DWORD window_style = WS_OVERLAPPEDWINDOW;

    int width  = description.width;
    int height = description.height;

    RECT border_rect = { 0 };
    AdjustWindowRectEx(&border_rect, window_style, 0, 0);
    width  += border_rect.right  - border_rect.left;
    height += border_rect.bottom - border_rect.top;

    struct Window w = { 0 };

    w.state = calloc(1, sizeof(struct Window_State)); // @libc
    memset(w.state, 0, sizeof(struct Window_State));  // @libc
    w.state->width  = width;
    w.state->height = height;

    w.handle = CreateWindowExA(
        0, class.lpszClassName,
        description.title,
        window_style,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, current_process_handle, w.state
    );

    w.device_context = GetDC(w.handle);

    if (description.backend == Graphics_Backend_OpenGL) {
        window_opengl_set_rendering_context(w.handle, w.device_context, opengl_pixel_format);


        if (!sogl_loadOpenGL()) {
            const char **failures = sogl_getFailures();
            int i = 1;
            while (*failures) {
                fprintf(stderr, "Failed to load function %s\n", *failures);
                failures++;
            }
        }
    }

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

// @Cleaning: In raylib, the `begin` calls accept parameters,
// and the `end` calls just operate with whatever the `begin` call was given.
// So if `begin` receives a window, that window should be the one that is used by `end`.
// What could be useful in this scenario is a hashmap on the thread context to store arbitrary global data.
static thread_local struct Window *currently_drawing_window = NULL;

void window_draw_begin(struct Window *window) {
    currently_drawing_window = window;
}

void window_draw_end(void) {
    struct Window *window = currently_drawing_window;

    MSG msg = { 0 };
    if (PeekMessageA(&msg, window->handle, 0, 0, PM_REMOVE)) {
        bool should_process = msg.message != WM_QUIT;
        if (should_process) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }


    SwapBuffers(currently_drawing_window->device_context);

    currently_drawing_window = NULL;
}


#endif
