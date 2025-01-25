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

struct Colored_Vertex   { float x, y, z,    r, g, b, a,   s, t; };
struct Colored_Triangle { struct Colored_Vertex vertices[3]; };

void colored_vertex_bind_attributes(void) {
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *) (7 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

struct Render_State {
    size_t transforms_count;
    mat4 transforms[256];

    size_t render_batch_count;
    struct Colored_Triangle render_batch[1024];
};

static struct Render_State render_state;

static struct Colored_Vertex vertices[] = {
    { .x = -0.5f, .y = -0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 0.0f, },
    { .x =  0.5f, .y = -0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 0.0f, },
    { .x =  0.5f, .y =  0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 1.0f, },
    { .x =  0.5f, .y =  0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 1.0f, },
    { .x = -0.5f, .y =  0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 1.0f, },
    { .x = -0.5f, .y = -0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 0.0f, },

    { .x = -0.5f, .y = -0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 0.0f, },
    { .x =  0.5f, .y = -0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 0.0f, },
    { .x =  0.5f, .y =  0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 1.0f, },
    { .x =  0.5f, .y =  0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 1.0f, },
    { .x = -0.5f, .y =  0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 1.0f, },
    { .x = -0.5f, .y = -0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 0.0f, },

    { .x = -0.5f, .y =  0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 0.0f, },
    { .x = -0.5f, .y =  0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 1.0f, },
    { .x = -0.5f, .y = -0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 1.0f, },
    { .x = -0.5f, .y = -0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 1.0f, },
    { .x = -0.5f, .y = -0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 0.0f, },
    { .x = -0.5f, .y =  0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 0.0f, },

    { .x =  0.5f, .y =  0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 0.0f, },
    { .x =  0.5f, .y =  0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 1.0f, },
    { .x =  0.5f, .y = -0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 1.0f, },
    { .x =  0.5f, .y = -0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 1.0f, },
    { .x =  0.5f, .y = -0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 0.0f, },
    { .x =  0.5f, .y =  0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 0.0f, },

    { .x = -0.5f, .y = -0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 1.0f, },
    { .x =  0.5f, .y = -0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 1.0f, },
    { .x =  0.5f, .y = -0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 0.0f, },
    { .x =  0.5f, .y = -0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 0.0f, },
    { .x = -0.5f, .y = -0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 0.0f, },
    { .x = -0.5f, .y = -0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 1.0f, },

    { .x = -0.5f, .y =  0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 1.0f, },
    { .x =  0.5f, .y =  0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 1.0f, },
    { .x =  0.5f, .y =  0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 0.0f, },
    { .x =  0.5f, .y =  0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 1.0f, .t = 0.0f, },
    { .x = -0.5f, .y =  0.5f, .z =  0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 0.0f, },
    { .x = -0.5f, .y =  0.5f, .z = -0.5f,   .r = 1.f, .g = 1.f, .b = 1.f,   .s = 0.0f, .t = 1.0f, },
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

const static struct Color text_foreground = { .r = 0xF0, .g = 0xF0, .b = 0xCB, .a = 0xFF };

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

static const struct Image texture_default_image = {
    .width = 1, .height = 1, .channels_count = 4,
    .data  = (unsigned char[4]) {
        0xFF, 0xFF, 0xFF, 0xFF,
    },
};

unsigned int load_texture_from_image_file(const char *file_path, bool generate_mipmaps);
unsigned int load_texture_from_image(struct Image image, bool generate_mipmaps);
unsigned int shader_compile_from_file(const char *path, GLenum shader_type, struct Allocator *allocator);
unsigned int shader_program_compile_from_files(const char *vs_path, const char *fs_path);

struct Vector2 { float x, y; };

void draw_triangle(struct Vector2 v1, struct Vector2 v2, struct Vector2 v3, struct Color color);
void draw_triangle(struct Vector2 v1, struct Vector2 v2, struct Vector2 v3, struct Color color) {
    struct Colored_Triangle *triangle = &render_state.render_batch[render_state.render_batch_count++];
    triangle->vertices[0] = (struct Colored_Vertex) { v1.x, v1.y, 0.0f,   color.r, color.g, color.b, color.a,   0, 0 };
    triangle->vertices[1] = (struct Colored_Vertex) { v2.x, v2.y, 0.0f,   color.r, color.g, color.b, color.a,   0, 0 };
    triangle->vertices[2] = (struct Colored_Vertex) { v3.x, v3.y, 0.0f,   color.r, color.g, color.b, color.a,   0, 0 };
}

static inline bool render_should_transform(void) {
    return render_state.transforms_count > 0;
}

void render_transform_push(mat4 matrix_value) {
    mat4 *m = &render_state.transforms[render_state.transforms_count++];
    *m = matrix_value;
    render_state.current_matrix = m;
}

mat4 render_transform_pop(void) {
    return render_state.transforms[--render_state.transforms_count];
}

void render_translate(vec3 translation) {
    glm_translate(render_state.current_matrix, translation);
}

void draw_cube(vec3 position) {

    float x = position.x;
    float y = position.y;
    float z = position.z;

    // https://github.com/raysan5/raylib/blob/7bfc8e8ca75882de434c601c4294ca1774b69278/src/rmodels.c#L257
    render_transform_push(GLM_MAT4_IDENTITY);
        render_translate(position);
        render_begin_geometry(GL_TRIANGLES);
            float half_width  = width  / 2.f
            float half_height = height / 2.f
            float half_length = length / 2.f
            render_vertex_textured(half_width, half_height, half_length,   0, 0);
            render_vertex_textured(half_width, half_height, half_length,   0, 0);
        render_end_geomentry();
    render_transform_pop();

    {
        struct Colored_Triangle *t11 = &render_state.render_batch[render_state.render_batch_count++];
        t11->vertices[0] = (struct Colored_Vertex) { -0.5f, -0.5f, -0.5f,   1.f, 1.f, 1.f, 1.f,  0.0f, 0.0f, };
        t11->vertices[1] = (struct Colored_Vertex) {  0.5f, -0.5f, -0.5f,   1.f, 1.f, 1.f, 1.f,  1.0f, 0.0f, };
        t11->vertices[2] = (struct Colored_Vertex) {  0.5f,  0.5f, -0.5f,   1.f, 1.f, 1.f, 1.f,  1.0f, 1.0f, };

        struct Colored_Triangle *t12 = &render_state.render_batch[render_state.render_batch_count++];
        t12->vertices[0] = (struct Colored_Vertex) {  0.5f,  0.5f, -0.5f,   1.f, 1.f, 1.f, 1.f,  1.0f, 1.0f, };
        t12->vertices[1] = (struct Colored_Vertex) { -0.5f,  0.5f, -0.5f,   1.f, 1.f, 1.f, 1.f,  0.0f, 1.0f, };
        t12->vertices[2] = (struct Colored_Vertex) { -0.5f, -0.5f, -0.5f,   1.f, 1.f, 1.f, 1.f,  0.0f, 0.0f, };
    }
    {
        struct Colored_Triangle *t21 = &render_state.render_batch[render_state.render_batch_count++];
        t21->vertices[0] = (struct Colored_Vertex) { -0.5f, -0.5f, 0.5f,   1.f, 1.f, 1.f, 1.f,   0.0f, 0.0f, };
        t21->vertices[1] = (struct Colored_Vertex) {  0.5f, -0.5f, 0.5f,   1.f, 1.f, 1.f, 1.f,   1.0f, 0.0f, };
        t21->vertices[2] = (struct Colored_Vertex) {  0.5f,  0.5f, 0.5f,   1.f, 1.f, 1.f, 1.f,   1.0f, 1.0f, };

        struct Colored_Triangle *t22 = &render_state.render_batch[render_state.render_batch_count++];
        t22->vertices[0] = (struct Colored_Vertex) {  0.5f,  0.5f,  0.5f,  1.f, 1.f, 1.f, 1.f,   1.0f, 1.0f, };
        t22->vertices[1] = (struct Colored_Vertex) { -0.5f,  0.5f,  0.5f,  1.f, 1.f, 1.f, 1.f,   0.0f, 1.0f, };
        t22->vertices[2] = (struct Colored_Vertex) { -0.5f, -0.5f,  0.5f,  1.f, 1.f, 1.f, 1.f,   0.0f, 0.0f, };
    }
    {
        struct Colored_Triangle *t31 = &render_state.render_batch[render_state.render_batch_count++];
        t31->vertices[0] = (struct Colored_Vertex) { -0.5f,  0.5f,  0.5f,  1.f, 1.f, 1.f, 1.f,   1.0f, 0.0f, };
        t31->vertices[0] = (struct Colored_Vertex) { -0.5f,  0.5f, -0.5f,  1.f, 1.f, 1.f, 1.f,   1.0f, 1.0f, };
        t31->vertices[0] = (struct Colored_Vertex) { -0.5f, -0.5f, -0.5f,  1.f, 1.f, 1.f, 1.f,   0.0f, 1.0f, };

        struct Colored_Triangle *t32 = &render_state.render_batch[render_state.render_batch_count++];
        t32->vertices[0] = (struct Colored_Vertex) { -0.5f, -0.5f, -0.5f,  1.f, 1.f, 1.f, 1.f,   0.0f, 1.0f, };
        t32->vertices[0] = (struct Colored_Vertex) { -0.5f, -0.5f,  0.5f,  1.f, 1.f, 1.f, 1.f,   0.0f, 0.0f, };
        t32->vertices[0] = (struct Colored_Vertex) { -0.5f,  0.5f,  0.5f,  1.f, 1.f, 1.f, 1.f,   1.0f, 0.0f, };
    }
    {
        struct Colored_Triangle *t41 = &render_state.render_batch[render_state.render_batch_count++];
        t41->vertices[0] = (struct Colored_Vertex) {  0.5f,  0.5f,  0.5f,  1.f, 1.f, 1.f, 1.f,   1.0f, 0.0f, };
        t41->vertices[0] = (struct Colored_Vertex) {  0.5f,  0.5f, -0.5f,  1.f, 1.f, 1.f, 1.f,   1.0f, 1.0f, };
        t41->vertices[0] = (struct Colored_Vertex) {  0.5f, -0.5f, -0.5f,  1.f, 1.f, 1.f, 1.f,   0.0f, 1.0f, };

        struct Colored_Triangle *t42 = &render_state.render_batch[render_state.render_batch_count++];
        t42->vertices[0] = (struct Colored_Vertex) {  0.5f, -0.5f, -0.5f,  1.f, 1.f, 1.f, 1.f,   0.0f, 1.0f, };
        t42->vertices[0] = (struct Colored_Vertex) {  0.5f, -0.5f,  0.5f,  1.f, 1.f, 1.f, 1.f,   0.0f, 0.0f, };
        t42->vertices[0] = (struct Colored_Vertex) {  0.5f,  0.5f,  0.5f,  1.f, 1.f, 1.f, 1.f,   1.0f, 0.0f, };
    }
    {
        struct Colored_Triangle *t51 = &render_state.render_batch[render_state.render_batch_count++];
        t51->vertices[0] = (struct Colored_Vertex) { -0.5f, -0.5f, -0.5f,  1.f, 1.f, 1.f, 1.f,   0.0f, 1.0f, };
        t51->vertices[0] = (struct Colored_Vertex) {  0.5f, -0.5f, -0.5f,  1.f, 1.f, 1.f, 1.f,   1.0f, 1.0f, };
        t51->vertices[0] = (struct Colored_Vertex) {  0.5f, -0.5f,  0.5f,  1.f, 1.f, 1.f, 1.f,   1.0f, 0.0f, };

        struct Colored_Triangle *t52 = &render_state.render_batch[render_state.render_batch_count++];
        t52->vertices[0] = (struct Colored_Vertex) {  0.5f, -0.5f,  0.5f,  1.f, 1.f, 1.f, 1.f,   1.0f, 0.0f, };
        t52->vertices[0] = (struct Colored_Vertex) { -0.5f, -0.5f,  0.5f,  1.f, 1.f, 1.f, 1.f,   0.0f, 0.0f, };
        t52->vertices[0] = (struct Colored_Vertex) { -0.5f, -0.5f, -0.5f,  1.f, 1.f, 1.f, 1.f,   0.0f, 1.0f, };
    }
    {
        struct Colored_Triangle *t61 = &render_state.render_batch[render_state.render_batch_count++];
        t61->vertices[0] = (struct Colored_Vertex) { -0.5f,  0.5f, -0.5f,  1.f, 1.f, 1.f, 1.f,   0.0f, 1.0f, };
        t61->vertices[0] = (struct Colored_Vertex) {  0.5f,  0.5f, -0.5f,  1.f, 1.f, 1.f, 1.f,   1.0f, 1.0f, };
        t61->vertices[0] = (struct Colored_Vertex) {  0.5f,  0.5f,  0.5f,  1.f, 1.f, 1.f, 1.f,   1.0f, 0.0f, };

        struct Colored_Triangle *t62 = &render_state.render_batch[render_state.render_batch_count++];
        t62->vertices[0] = (struct Colored_Vertex) {  0.5f,  0.5f,  0.5f,  1.f, 1.f, 1.f, 1.f,   1.0f, 0.0f, };
        t62->vertices[0] = (struct Colored_Vertex) { -0.5f,  0.5f,  0.5f,  1.f, 1.f, 1.f, 1.f,   0.0f, 0.0f, };
        t62->vertices[0] = (struct Colored_Vertex) { -0.5f,  0.5f, -0.5f,  1.f, 1.f, 1.f, 1.f,   0.0f, 1.0f, };
    }
}

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
    unsigned int texture_default = load_texture_from_image(texture_default_image, false);

    /// Shaders
    
    unsigned int shaderProgram = shader_program_compile_from_files(
        "test/default.vert",
        "test/default.frag"
    );

    /// Buffers


    unsigned int vao, vbo;
    {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao); {
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            colored_vertex_bind_attributes();

        } glBindVertexArray(0);
    }

    unsigned int batch_vao, batch_vbo;
    {
        glGenVertexArrays(1, &batch_vao);
        glBindVertexArray(batch_vao); {

            glGenBuffers(1, &batch_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, batch_vbo);
            colored_vertex_bind_attributes();

        } glBindVertexArray(0);
    }

    glViewport(0, 0, initial_width, initial_height);

    glUseProgram(shaderProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_missing);

    unsigned int model_uniform      = glGetUniformLocation(shaderProgram, "model");
    unsigned int view_uniform       = glGetUniformLocation(shaderProgram, "view");
    unsigned int projection_uniform = glGetUniformLocation(shaderProgram, "projection");

    double start_time_seconds = win32_get_system_timestamp() / 1e7;

    while (!window_should_close(window)) {
        double time = win32_get_system_timestamp() / 1e7;
        double seconds_since_start = time - start_time_seconds;

        window_draw_begin(&window); {
            draw_background_clear((struct Color) { 0.2f, 0.3f, 0.3f, 1.0f });

            mat4 identity = GLM_MAT4_IDENTITY;

            glUniformMatrix4fv(model_uniform,      1, GL_FALSE, identity[0]);
            glUniformMatrix4fv(view_uniform,       1, GL_FALSE, identity[0]);
            glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, identity[0]);

            draw_triangle(
                (struct Vector2) {  0.f,  -25.f },
                (struct Vector2) {  25.f,  25.f },
                (struct Vector2) { -25.f,  25.f },
                text_foreground
            );

            draw_triangle(
                (struct Vector2) {  0.f  + 50.f,  -25.f + 50.f },
                (struct Vector2) {  25.f + 50.f,   25.f + 50.f },
                (struct Vector2) { -25.f + 50.f,   25.f + 50.f },
                text_foreground
            );

            {
                glUseProgram(shaderProgram);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture_default);

                mat4 projection;
                glm_ortho(
                    initial_width  / -2.0f, initial_width  /  2.0f,
                    initial_height / -2.0f, initial_height /  2.0f,
                    0.0f, 1.0f,
                    projection
                );

                glUniformMatrix4fv(projection_uniform,  1, GL_FALSE, projection[0]);

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

                glBindTexture(GL_TEXTURE_2D, 0);
            }

            {
                glUseProgram(shaderProgram);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture_missing);

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
                glm_perspective(
                    glm_rad(45.0f),
                    initial_width / (float) initial_height,
                    0.1f, 100.0f,
                    projection
                );

                glBindVertexArray(vao); {

                    glBindBuffer(GL_ARRAY_BUFFER, vbo);

                    glUniformMatrix4fv(view_uniform,       1, GL_FALSE, view[0]);
                    glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, projection[0]);

                    for (size_t i = 0; i < 10; i++) {
                        mat4 model = GLM_MAT4_IDENTITY;
                        glm_translate(model, cube_positions[i]);
                        float angle = 20.0f * i; 
                        glm_rotate(model, glm_rad(angle), (vec3) { 1.0f, 0.3f, 0.5f, });

                        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, model[0]);
                        glDrawArrays(GL_TRIANGLES, 0, 36);
                    }

                } glBindVertexArray(0);

                glBindTexture(GL_TEXTURE_2D, 0);
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
