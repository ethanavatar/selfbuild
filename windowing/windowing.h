#ifndef WINDOWING_H
#define WINDOWING_H

#include <windows.h>

enum OpenGL_Profile {
    OpenGL_Profile_None,
    OpenGL_Profile_Core,
};

enum Graphics_Backend {
    Graphics_Backend_None,
    Graphics_Backend_OpenGL,
};

struct OpenGL_Description {
    enum OpenGL_Profile profile;
    int major_version, minor_version;
};

struct Backend_Description {
    enum Graphics_Backend kind;
    union {
        struct OpenGL_Description opengl;
    };
};

struct Window_State {
    bool destroyed;
    bool should_close;
    int width, height;
    struct Backend_Description backend;

    void *device_context; // win32 HDC
    PIXELFORMATDESCRIPTOR pixel_format_descriptor;
    HGLRC rendering_context;
};

struct Window {
    void *handle; // win32 HWND

    // @TODO: Windows should probably have an arena of their own to allocate this state struct
    struct Window_State *state;
};

struct Window_Description {
    char *title;
    int width, height;
    struct Backend_Description backend;
};


struct Window window_create(struct Window_Description);
void window_destroy(struct Window *);

bool window_should_close(struct Window);

void window_draw_begin(struct Window *);
void window_draw_end(void);

#endif // WINDOWING_H
