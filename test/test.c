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

#include "cglm/cglm.h"

inline static int64_t win32_get_system_timestamp(void) {
    FILETIME file_time;
    GetSystemTimeAsFileTime(&file_time);
    ULARGE_INTEGER uli;
    uli.LowPart = file_time.dwLowDateTime;
    uli.HighPart = file_time.dwHighDateTime;
    return (int64_t) uli.QuadPart;
}

const int initial_width  = 800;
const int initial_height = 600;

struct Vertex { float x, y, z,   s, t; };
static struct Vertex vertices[] = {
    { .x = -0.5f, .y = -0.5f, .z = -0.5f,   .s = 0.0f, .t = 0.0f, },
    { .x =  0.5f, .y = -0.5f, .z = -0.5f,   .s = 1.0f, .t = 0.0f, },
    { .x =  0.5f, .y =  0.5f, .z = -0.5f,   .s = 1.0f, .t = 1.0f, },
    { .x =  0.5f, .y =  0.5f, .z = -0.5f,   .s = 1.0f, .t = 1.0f, },
    { .x = -0.5f, .y =  0.5f, .z = -0.5f,   .s = 0.0f, .t = 1.0f, },
    { .x = -0.5f, .y = -0.5f, .z = -0.5f,   .s = 0.0f, .t = 0.0f, },

    { .x = -0.5f, .y = -0.5f, .z =  0.5f,   .s = 0.0f, .t = 0.0f, },
    { .x =  0.5f, .y = -0.5f, .z =  0.5f,   .s = 1.0f, .t = 0.0f, },
    { .x =  0.5f, .y =  0.5f, .z =  0.5f,   .s = 1.0f, .t = 1.0f, },
    { .x =  0.5f, .y =  0.5f, .z =  0.5f,   .s = 1.0f, .t = 1.0f, },
    { .x = -0.5f, .y =  0.5f, .z =  0.5f,   .s = 0.0f, .t = 1.0f, },
    { .x = -0.5f, .y = -0.5f, .z =  0.5f,   .s = 0.0f, .t = 0.0f, },

    { .x = -0.5f, .y =  0.5f, .z =  0.5f,   .s = 1.0f, .t = 0.0f, },
    { .x = -0.5f, .y =  0.5f, .z = -0.5f,   .s = 1.0f, .t = 1.0f, },
    { .x = -0.5f, .y = -0.5f, .z = -0.5f,   .s = 0.0f, .t = 1.0f, },
    { .x = -0.5f, .y = -0.5f, .z = -0.5f,   .s = 0.0f, .t = 1.0f, },
    { .x = -0.5f, .y = -0.5f, .z =  0.5f,   .s = 0.0f, .t = 0.0f, },
    { .x = -0.5f, .y =  0.5f, .z =  0.5f,   .s = 1.0f, .t = 0.0f, },

    { .x =  0.5f, .y =  0.5f, .z =  0.5f,   .s = 1.0f, .t = 0.0f, },
    { .x =  0.5f, .y =  0.5f, .z = -0.5f,   .s = 1.0f, .t = 1.0f, },
    { .x =  0.5f, .y = -0.5f, .z = -0.5f,   .s = 0.0f, .t = 1.0f, },
    { .x =  0.5f, .y = -0.5f, .z = -0.5f,   .s = 0.0f, .t = 1.0f, },
    { .x =  0.5f, .y = -0.5f, .z =  0.5f,   .s = 0.0f, .t = 0.0f, },
    { .x =  0.5f, .y =  0.5f, .z =  0.5f,   .s = 1.0f, .t = 0.0f, },

    { .x = -0.5f, .y = -0.5f, .z = -0.5f,   .s = 0.0f, .t = 1.0f, },
    { .x =  0.5f, .y = -0.5f, .z = -0.5f,   .s = 1.0f, .t = 1.0f, },
    { .x =  0.5f, .y = -0.5f, .z =  0.5f,   .s = 1.0f, .t = 0.0f, },
    { .x =  0.5f, .y = -0.5f, .z =  0.5f,   .s = 1.0f, .t = 0.0f, },
    { .x = -0.5f, .y = -0.5f, .z =  0.5f,   .s = 0.0f, .t = 0.0f, },
    { .x = -0.5f, .y = -0.5f, .z = -0.5f,   .s = 0.0f, .t = 1.0f, },

    { .x = -0.5f, .y =  0.5f, .z = -0.5f,   .s = 0.0f, .t = 1.0f, },
    { .x =  0.5f, .y =  0.5f, .z = -0.5f,   .s = 1.0f, .t = 1.0f, },
    { .x =  0.5f, .y =  0.5f, .z =  0.5f,   .s = 1.0f, .t = 0.0f, },
    { .x =  0.5f, .y =  0.5f, .z =  0.5f,   .s = 1.0f, .t = 0.0f, },
    { .x = -0.5f, .y =  0.5f, .z =  0.5f,   .s = 0.0f, .t = 0.0f, },
    { .x = -0.5f, .y =  0.5f, .z = -0.5f,   .s = 0.0f, .t = 1.0f, },
};

static vec3 cube_positions[] = {
    {  0.0f,  0.0f,  0.00f, }, 
    {  2.0f,  5.0f, -15.0f, }, 
    { -1.5f, -2.2f, -2.50f, },  
    { -3.8f, -2.0f, -12.3f, },  
    {  2.4f, -0.4f, -3.50f, },  
    { -1.7f,  3.0f, -7.50f, },  
    {  1.3f, -2.0f, -2.50f, },  
    {  1.5f,  2.0f, -2.50f, }, 
    {  1.5f,  0.2f, -1.50f, }, 
    { -1.3f,  1.0f, -1.50f, },
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

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glViewport(0, 0, initial_width, initial_height);

    glUseProgram(shaderProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_missing);

    unsigned int model_uniform      = glGetUniformLocation(shaderProgram, "model");
    unsigned int view_uniform       = glGetUniformLocation(shaderProgram, "view");
    unsigned int projection_uniform = glGetUniformLocation(shaderProgram, "projection");

    double start_time_seconds = win32_get_system_timestamp() / 1e7;

    vec3 camera_position  = (vec3) { 0.0f, 0.0f, 3.0f };
    vec3 camera_target    = (vec3) { 0.0f, 0.0f, 0.0f };
    vec3 camera_direction;
    glm_vec3_sub(camera_position, camera_target, camera_direction);
    glm_vec3_normalize(camera_direction);

    vec3 up = GLM_YUP;
    vec3 camera_right;
    glm_vec3_cross(up, camera_direction, camera_right);
    glm_vec3_normalize(camera_right);

    vec3 camera_up;
    glm_vec3_cross(camera_direction, camera_right, camera_up);

    while (!window_should_close(window)) {
        double time = win32_get_system_timestamp() / 1e7;
        double seconds_since_start = time - start_time_seconds;

        window_draw_begin(&window); {
            draw_background_clear((struct Color) { 0.2f, 0.3f, 0.3f, 1.0f });

            {
                const float radius = 10.f;
                float camera_x = sin(seconds_since_start) * radius;
                float camera_z = cos(seconds_since_start) * radius;

                mat4 view;
                glm_lookat(
                    (vec3) { camera_x, 0.0f, camera_z },
                    (vec3) { 0.0f, 0.0f, 0.0f },
                    (vec3) { 0.0f, 1.0f, 0.0f },
                    view
                );

                mat4 projection;
                glm_perspective(glm_rad(45.0f), initial_width / (float) initial_height, 0.1f, 100.0f, projection);

                glUniformMatrix4fv(view_uniform,       1, GL_FALSE, view[0]);
                glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, projection[0]);


                glBindVertexArray(vao); {

                    for (size_t i = 0; i < 10; i++) {
                        mat4 model = GLM_MAT4_IDENTITY;
                        glm_translate(model, cube_positions[i]);
                        float angle = 20.0f * i; 
                        glm_rotate(model, glm_rad(angle), (vec3) { 1.0f, 0.3f, 0.5f, });
                        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, model[0]);

                        glDrawArrays(GL_TRIANGLES, 0, 36);
                    }

                } glBindVertexArray(0);
            }

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

