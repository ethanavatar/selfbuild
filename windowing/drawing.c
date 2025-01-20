#include "drawing.h"
#include "simple-opengl-loader/simple-opengl-loader.h"

void draw_background_clear(struct Color color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}
