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

struct Colored_Vertex   { float x, y, z,   r, g, b; };
struct Colored_Triangle { struct Colored_Vertex vertices[3]; };

struct Render_State {
    size_t render_batch_count;
    struct Colored_Triangle render_batch[1024];
};

static struct Render_State render_state;


struct Textured_Vertex { float x, y, z,   s, t; };
static struct Textured_Vertex vertices[] = {
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

struct Image {
    int width, height, channels_count;
    unsigned char *data;
};

static const struct Image texture_missing_image = {
    .width = 2, .height = 2, .channels_count = 4,
    .data  = (unsigned char[16]) {
        0xFF, 0x00, 0xFF, 0xFF,   0x00, 0x00, 0x00, 0xFF,
        0x00, 0x00, 0x00, 0xFF,   0xFF, 0x00, 0xFF, 0xFF,
    },
};

unsigned int load_texture_from_image_file(const char *file_path, bool generate_mipmaps);
unsigned int load_texture_from_image(struct Image image, bool generate_mipmaps);
unsigned int shader_compile_from_file(const char *path, GLenum shader_type, struct Allocator *allocator);
unsigned int shader_program_compile_from_files(const char *vs_path, const char *fs_path);

int main(void) {

    struct Thread_Context tctx;
    thread_context_init_and_equip(&tctx);

    render_state = (struct Render_State) { 0 };

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

    unsigned int texture_missing = load_texture_from_image(texture_missing_image, false);

    /// Shaders
    
    unsigned int shaderProgram = shader_program_compile_from_files("test/quad.vert", "test/quad.frag");
    unsigned int batchProgram  = shader_program_compile_from_files(
        "test/colored_verts_2d.vert",
        "test/colored_verts_2d.frag"
    );

    /// Buffers


    unsigned int vao, vbo;
    {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    unsigned int batch_vao, batch_vbo;
    {
        glGenVertexArrays(1, &batch_vao);
        glBindVertexArray(batch_vao); {

            glGenBuffers(1, &batch_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, batch_vbo);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) 0);
            glEnableVertexAttribArray(0);

            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) (3 * sizeof(float)));
            glEnableVertexAttribArray(1);

        } glBindVertexArray(0);
    }

    glViewport(0, 0, initial_width, initial_height);

    glUseProgram(shaderProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_missing);

    unsigned int model_uniform      = glGetUniformLocation(shaderProgram, "model");
    unsigned int view_uniform       = glGetUniformLocation(shaderProgram, "view");
    unsigned int projection_uniform = glGetUniformLocation(shaderProgram, "projection");

    glUseProgram(batchProgram);

    unsigned int batch_transform_uniform  = glGetUniformLocation(batchProgram, "transform");
    unsigned int batch_projection_uniform = glGetUniformLocation(batchProgram, "projection");

    double start_time_seconds = win32_get_system_timestamp() / 1e7;

    /*
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
    */

    while (!window_should_close(window)) {
        double time = win32_get_system_timestamp() / 1e7;
        double seconds_since_start = time - start_time_seconds;

        window_draw_begin(&window); {
            draw_background_clear((struct Color) { 0.2f, 0.3f, 0.3f, 1.0f });

            struct Colored_Triangle *triangle = &render_state.render_batch[render_state.render_batch_count++];
            triangle->vertices[0] = (struct Colored_Vertex) { -0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f };
            triangle->vertices[1] = (struct Colored_Vertex) {  0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f };
            triangle->vertices[2] = (struct Colored_Vertex) {  0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f };

            {
                glUseProgram(batchProgram);

                mat4 transform = GLM_MAT4_IDENTITY;
                glm_translate(transform, (vec3) { -0.75f, 0.75f, 0.0f });
                glm_scale(transform, (vec3) { 0.25f, 0.25f, 0.0f });

                mat4 projection = GLM_MAT4_IDENTITY;
                //glm_ortho(glm_rad(45.0f), initial_width / (float) initial_height, 0.1f, 100.0f, projection);

                glUniformMatrix4fv(batch_transform_uniform,  1, GL_FALSE, transform[0]);
                glUniformMatrix4fv(batch_projection_uniform, 1, GL_FALSE, projection[0]);

                glBindVertexArray(batch_vao); {
                    glBindBuffer(GL_ARRAY_BUFFER, batch_vbo);
                    if (render_state.render_batch_count > 0) {
                        glBufferData(
                            GL_ARRAY_BUFFER,
                            sizeof(struct Colored_Triangle) * render_state.render_batch_count,
                            render_state.render_batch,
                            GL_STATIC_DRAW
                        );
                        glDrawArrays(GL_TRIANGLES, 0, 3 * render_state.render_batch_count);
                    }
                    render_state.render_batch_count = 0;
                } glBindVertexArray(0);
            }

            {
                glUseProgram(shaderProgram);

                const float radius = 20.f;
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

                    glBindBuffer(GL_ARRAY_BUFFER, vbo);

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

unsigned int load_texture_from_image_file(const char *file_path, bool generate_mipmaps) {
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
    } else {
        fprintf(stderr, "failed to load texture from: %s", file_path);
    }

    if (data && generate_mipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    stbi_image_free(data);
    return texture;
}

unsigned int load_texture_from_image(struct Image image, bool generate_mipmaps) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    if (image.data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);
    }

    if (image.data && generate_mipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    return texture;
}

unsigned int shader_compile_from_file(
    const char *path, GLenum shader_type,
    struct Allocator *allocator
) {
    unsigned int shader_id;
    struct File_Contents source = file_read_to_end(path, allocator);

    shader_id = glCreateShader(shader_type);
    glShaderSource(shader_id, 1, &source.contents, NULL);
    glCompileShader(shader_id);

    int success;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        fprintf(stderr, "failed to compile `%s`\n", path);

        char info_log[512] = { 0 };
        glGetShaderInfoLog(shader_id, 512, NULL, info_log);
        fprintf(stderr, "%s", info_log);

        assert(false);
    }

    return shader_id;
}

unsigned int shader_program_compile_from_files(const char *vs_path, const char *fs_path) {
    struct Allocator scratch = scratch_begin();
    unsigned int vs_id = shader_compile_from_file(vs_path, GL_VERTEX_SHADER,   &scratch);
    unsigned int fs_id = shader_compile_from_file(fs_path, GL_FRAGMENT_SHADER, &scratch);

    unsigned int shader_program_id = glCreateProgram();

    glAttachShader(shader_program_id, vs_id);
    glAttachShader(shader_program_id, fs_id);
    glLinkProgram(shader_program_id);

    int success;
    glGetProgramiv(shader_program_id, GL_LINK_STATUS, &success);
    assert(success);

    glDeleteShader(fs_id);
    glDeleteShader(vs_id);

    scratch_end(&scratch);
    return shader_program_id;
}
