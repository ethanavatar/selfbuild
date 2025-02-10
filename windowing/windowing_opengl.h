#ifndef WINDOWING_OPENGL_H_
#define WINDOWING_OPENGL_H_

#include <windows.h>

PIXELFORMATDESCRIPTOR window_opengl_set_pixel_format(
    HINSTANCE instance, WNDCLASSA class
);

HGLRC window_opengl_set_rendering_context(
    HWND window_handle, HDC device_context, PIXELFORMATDESCRIPTOR pixel_format_descriptor
);

#endif // WINDOWING_OPENGL_H_

