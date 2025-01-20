#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include <windows.h>

#include "stdlib/allocators.h"
#include "stdlib/thread_context.h"
#include "stdlib/scratch_memory.h"
#include "stdlib/file_io.h"

#include "windowing/windowing.h"
#include "windowing/drawing.h"

#define SOGL_MAJOR_VERSION 4
#define SOGL_MINOR_VERSION 6
#define SOGL_IMPLEMENTATION_WIN32
#include "simple-opengl-loader/simple-opengl-loader.h"

const int initial_width  = 800;
const int initial_height = 600;

static float vertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};

int main(void) {

    struct Thread_Context tctx;
    thread_context_init_and_equip(&tctx);

    struct Allocator persistent = scratch_begin();

    struct OpenGL_Description opengl_description = {
        .profile = OpenGL_Profile_Core,
        .major_version = SOGL_MAJOR_VERSION, .minor_version = SOGL_MINOR_VERSION,
    };

    struct Window_Description window_description = {
        .title   = "Window Title",
        .width   = initial_width,
        .height  = initial_height,
        .backend = Graphics_Backend_OpenGL,
        .opengl  = opengl_description,
    };

    struct Window window = window_create(window_description);
    if (!sogl_loadOpenGL()) {
        const char **failures = sogl_getFailures();
        int i = 1;
        while (*failures) {
            fprintf(stderr, "Failed to load function %s\n", *failures);
            failures++;
        }
    }

    /// Shaders

    struct Allocator shaders_scratch = scratch_begin();
    unsigned int vertexShader;
    {
        struct File_Contents vertex_shader_source = file_read_to_end("test/vert.glsl", &shaders_scratch);

        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertex_shader_source.contents, NULL);
        glCompileShader(vertexShader);

        int success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        assert(success);
    }

    unsigned int fragmentShader;
    {
        struct File_Contents fragment_shader_source = file_read_to_end("test/frag.glsl", &shaders_scratch);

        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragment_shader_source.contents, NULL);
        glCompileShader(fragmentShader);

        int success;
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        assert(success);
    }

    unsigned int shaderProgram;
    {
        shaderProgram = glCreateProgram();

        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        int success;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        assert(success);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    scratch_end(&shaders_scratch);

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

    glViewport(0, 0, initial_width, initial_height);

    while (!window_should_close(window)) {
        window_draw_begin(&window); {
            draw_background_clear((struct Color) { 0.2f, 0.3f, 0.3f, 1.0f });

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

    scratch_end(&persistent);
    thread_context_release();
    return 0;
}

