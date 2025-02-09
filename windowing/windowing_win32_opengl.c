#include <windows.h>
#include <assert.h>

#include <gl/gl.h>
#include "gl/wglext.h"

#include "windowing/windowing.h"

static PFNWGLCHOOSEPIXELFORMATARBPROC    wglChoosePixelFormatARB;
static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

PIXELFORMATDESCRIPTOR window_opengl_set_pixel_format(
    HINSTANCE instance, WNDCLASSA class
) {
    struct Window_State dummy_state;
    HWND window = CreateWindow(
        class.lpszClassName, "Dummy Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, instance, &dummy_state
    );

    assert(window);

    HDC device_context = GetDC(window);
    assert(device_context);

    const PIXELFORMATDESCRIPTOR pixel_format_descriptor = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,

        .cColorBits = 32,
        .cAlphaBits = 8,
        .cDepthBits = 24,
        .cStencilBits = 8,
        .cAuxBuffers = 0,

        .iLayerType = PFD_MAIN_PLANE,
    };

    int pixel_format = ChoosePixelFormat(device_context, &pixel_format_descriptor);
    SetPixelFormat(device_context, pixel_format, &pixel_format_descriptor);

    HGLRC rendering_context = wglCreateContext(device_context);

    wglMakeCurrent(device_context, rendering_context);
    wglChoosePixelFormatARB    = (PFNWGLCHOOSEPIXELFORMATARBPROC)    (void *) wglGetProcAddress("wglChoosePixelFormatARB");
    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) (void *) wglGetProcAddress("wglCreateContextAttribsARB");

    wglMakeCurrent(device_context, NULL);
    wglDeleteContext(rendering_context);
    ReleaseDC(window, device_context);
    DestroyWindow(window);

    return pixel_format_descriptor;
}

HGLRC window_opengl_set_rendering_context(
    HWND window_handle, HDC device_context, PIXELFORMATDESCRIPTOR pixel_format_descriptor
) {
    int pixel_format = ChoosePixelFormat(device_context, &pixel_format_descriptor);

    const int wgl_pixel_attributes[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
        WGL_SWAP_METHOD_ARB,    WGL_SWAP_COPY_ARB,
        WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
        WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
        WGL_COLOR_BITS_ARB,     32,
        WGL_DEPTH_BITS_ARB,     24,
        WGL_STENCIL_BITS_ARB,   8,

        // https://stackoverflow.com/questions/34907832/activating-multisample-on-opengl-win32
        WGL_SAMPLES_ARB,        4,
        0
    };

    UINT pixel_format_count;
    wglChoosePixelFormatARB(device_context, wgl_pixel_attributes, NULL, 1, &pixel_format, &pixel_format_count);

    DescribePixelFormat(device_context, pixel_format, sizeof(PIXELFORMATDESCRIPTOR), &pixel_format_descriptor);
    SetPixelFormat(device_context, pixel_format, &pixel_format_descriptor);

    const int wgl_context_attributes[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 6,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };

    HGLRC rendering_context = wglCreateContextAttribsARB(device_context, 0, wgl_context_attributes);
    wglMakeCurrent(device_context, rendering_context);

    return rendering_context;
}

