#include "windowing/windowing.h"

const int initial_width  = 800;
const int initial_height = 600;

int main(void) {

    struct OpenGL_Description opengl_description = {
        .profile = OpenGL_Profile_Core,
        .major_version = 4, .minor_version = 6,
    };

    struct Window_Description window_description = {
        .title   = "Window Title",
        .width   = initial_width,
        .height  = initial_height,
        .backend = Graphics_Backend_OpenGL,
        .opengl  = opengl_description,
    };

    struct Window window = window_create(window_description);

    while (!window_should_close(window)) {
        window_draw_begin(&window);
        window_draw_end();
    }

    window_destroy(&window);
    return 0;
}
