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

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

const int initial_width  = 800;
const int initial_height = 600;

struct Vertex { float x, y, z,  r, g, b,  s, t; };
static struct Vertex vertices[] = {
    { .x =  0.5f, .y =  0.5f, .z = 0.0f,   .r = 1.0f, .g = 0.0f, .b = 0.0f,   .s = 1.0f, .t = 1.0f, },
    { .x =  0.5f, .y = -0.5f, .z = 0.0f,   .r = 0.0f, .g = 1.0f, .b = 0.0f,   .s = 1.0f, .t = 0.0f, },
    { .x = -0.5f, .y = -0.5f, .z = 0.0f,   .r = 0.0f, .g = 0.0f, .b = 1.0f,   .s = 0.0f, .t = 0.0f, },
    { .x = -0.5f, .y =  0.5f, .z = 0.0f,   .r = 1.0f, .g = 1.0f, .b = 0.0f,   .s = 0.0f, .t = 1.0f, },
};

static unsigned int indices[] = {
    0, 1, 3,
    1, 2, 3,
};

unsigned int load_texture_from_image(const char *file_path) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, channels_count;
    unsigned char *data = stbi_load(file_path, &width, &height, &channels_count, 0);

    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        //glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        fprintf(stderr, "failed to load texture from: %s", file_path);
    }

    stbi_image_free(data);
    return texture;
}

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

    unsigned int texture_missing = load_texture_from_image("test/assets/Texture_Missing.png");

    /// Shaders

    struct Allocator shaders_scratch = scratch_begin();
    unsigned int vertexShader;
    {
        struct File_Contents vertex_shader_source = file_read_to_end("test/quad.vert", &shaders_scratch);

        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertex_shader_source.contents, NULL);
        glCompileShader(vertexShader);

        int success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        assert(success);
    }

    unsigned int fragmentShader;
    {
        struct File_Contents fragment_shader_source = file_read_to_end("test/quad.frag", &shaders_scratch);

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

    unsigned int ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    glViewport(0, 0, initial_width, initial_height);

    /*
    int triangle_color_uniform = glGetUniformLocation(shaderProgram, "triangle_color");
    glUseProgram(shaderProgram);
    glUniform4f(triangle_color_uniform,
        0xF0 / 1.f,
        0xF0 / 1.f,
        0xCB / 1.f,
        0xFF / 1.f
    );
    */

    int texture_uniform = glGetUniformLocation(shaderProgram, "texture");
    glUniform4f(texture_uniform,
        0xF0 / 1.f,
        0xF0 / 1.f,
        0xCB / 1.f,
        0xFF / 1.f
    );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_missing);

    while (!window_should_close(window)) {
        window_draw_begin(&window); {
            draw_background_clear((struct Color) { 0.2f, 0.3f, 0.3f, 1.0f });

            glUseProgram(shaderProgram);
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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

