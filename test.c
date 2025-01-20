#include <assert.h>
#include <stdint.h>

#include "stdlib/thread_context.h"
#include "windowing/windowing.h"

#include <windows.h>
#include <wingdi.h>
#include <gl/gl.h>
#include <gl/wglext.h>

#include "opengl_definitions.h"

const int initial_width  = 800;
const int initial_height = 600;

typedef void (*Function_glShaderSource)    (unsigned int, unsigned int, const char **, const int *);
typedef void (*Function_glGenBuffers)      (int, unsigned int *);
typedef void (*Function_glBindBuffer)      (int, unsigned int);
typedef void (*Function_glBufferData)      (int, intptr_t, const void *, int);
typedef void (*Function_glCompileShader)   (unsigned int);
typedef void (*Function_glAttachShader)    (unsigned int, unsigned int);
typedef void (*Function_glLinkProgram)     (unsigned int);
typedef void (*Function_glDeleteShader)    (unsigned int);
typedef void (*Function_glGenVertexArrays) (int, unsigned int *);
typedef void (*Function_glVertexAttribPointer) (unsigned int, int, int, unsigned char, int, const void *);
typedef void (*Function_glEnableVertexAttribArray) (unsigned int);
typedef void (*Function_glUseProgram) (unsigned int);
typedef void (*Function_glBindVertexArray) (unsigned int);
typedef void (*Function_glDeleteBuffers) (int, const unsigned int *);
typedef void (*Function_glDeleteProgram) (unsigned int);
typedef void (*Function_glGetShaderiv)  (unsigned int, int, int *);
typedef void (*Function_glGetProgramiv) (unsigned int, int, int *);
typedef void (*Function_glDeleteVertexArrays) (int, const unsigned int *);

typedef unsigned int (*Function_glCreateShader) (int);
typedef unsigned int (*Function_glCreateProgram) (void);

Function_glShaderSource    glShaderSource     = NULL;
Function_glGenBuffers      glGenBuffers       = NULL;
Function_glBindBuffer      glBindBuffer       = NULL;
Function_glBufferData      glBufferData       = NULL;
Function_glCreateShader    glCreateShader     = NULL;
Function_glCompileShader   glCompileShader    = NULL;
Function_glCreateProgram   glCreateProgram    = NULL;
Function_glAttachShader    glAttachShader     = NULL;
Function_glLinkProgram     glLinkProgram      = NULL;
Function_glDeleteShader    glDeleteShader     = NULL;
Function_glGenVertexArrays glGenVertexArrays  = NULL;
Function_glVertexAttribPointer     glVertexAttribPointer  = NULL;
Function_glEnableVertexAttribArray glEnableVertexAttribArray  = NULL;
Function_glUseProgram      glUseProgram  = NULL;
Function_glBindVertexArray glBindVertexArray  = NULL;
Function_glDeleteBuffers   glDeleteBuffers  = NULL;
Function_glDeleteProgram   glDeleteProgram  = NULL;
Function_glGetShaderiv     glGetShaderiv  = NULL;
Function_glGetProgramiv    glGetProgramiv  = NULL;
Function_glDeleteVertexArrays glDeleteVertexArrays  = NULL;

void load_opengl_functions(void);

static float vertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};

static const char *vertexShaderSource =
    "#version 460 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main() {\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

static const char *fragmentShaderSource =
    "#version 460 core\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\0";


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
    load_opengl_functions();

    /// Shaders

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    assert(success);

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    assert(success);

    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    assert(success);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    /// Buffers

    unsigned int vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    unsigned int vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    while (!window_should_close(window)) {
        window_draw_begin(&window); {

            glViewport(0, 0, initial_width, initial_height);
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(shaderProgram);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);

        } window_draw_end();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);

    window_destroy(&window);
    return 0;
}

void load_opengl_functions(void) {
    glShaderSource     = (void *) wglGetProcAddress("glShaderSource");
    glGenBuffers       = (void *) wglGetProcAddress("glGenBuffers");
    glBindBuffer       = (void *) wglGetProcAddress("glBindBuffer");
    glBufferData       = (void *) wglGetProcAddress("glBufferData");
    glCreateShader     = (void *) wglGetProcAddress("glCreateShader");
    glCompileShader    = (void *) wglGetProcAddress("glCompileShader");
    glCreateProgram    = (void *) wglGetProcAddress("glCreateProgram");
    glAttachShader     = (void *) wglGetProcAddress("glAttachShader");
    glLinkProgram      = (void *) wglGetProcAddress("glLinkProgram");
    glDeleteShader     = (void *) wglGetProcAddress("glDeleteShader");
    glGenVertexArrays  = (void *) wglGetProcAddress("glGenVertexArrays");
    glVertexAttribPointer     = (void *) wglGetProcAddress("glVertexAttribPointer");
    glEnableVertexAttribArray = (void *) wglGetProcAddress("glEnableVertexAttribArray");
    glUseProgram       = (void *) wglGetProcAddress("glUseProgram");
    glBindVertexArray  = (void *) wglGetProcAddress("glBindVertexArray");
    glDeleteBuffers    = (void *) wglGetProcAddress("glDeleteBuffers");
    glDeleteProgram    = (void *) wglGetProcAddress("glDeleteProgram");
    glGetShaderiv      = (void *) wglGetProcAddress("glGetShaderiv");
    glGetProgramiv     = (void *) wglGetProcAddress("glGetProgramiv");
    glDeleteVertexArrays = (void *) wglGetProcAddress("glDeleteVertexArrays");
}
