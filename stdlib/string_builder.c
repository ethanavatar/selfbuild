#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "stdlib/string_builder.h"
#include "stdlib/allocators.h"

struct String_Builder string_builder_create(
    struct Allocator *allocator,
    size_t initial_capacity
) {
    struct String_Builder b = { 0 };
    b.allocator = allocator;
    b.capacity  = initial_capacity;
    b.buffer    = allocator_allocate(allocator, initial_capacity * sizeof(char));
    return b;
}

void string_builder_append_vargs(
    struct String_Builder *builder,
    const char *format, va_list format_args
) {
    size_t extra_length = vsnprintf(NULL, 0, format, format_args);
    bool needs_resize = builder->length + (extra_length + 1) > builder->capacity;

    if (needs_resize) {
        size_t new_capacity = builder->capacity == 0
            ? extra_length + 1
            : builder->capacity * 2;

        char *new_buffer = allocator_allocate(builder->allocator, new_capacity);

        if (builder->buffer) {
            // @TODO: My own memmove
            memmove(new_buffer, builder->buffer, builder->length);
            allocator_release(builder->allocator, builder->buffer);
        }

        builder->buffer = new_buffer;
        builder->capacity = new_capacity;
    }

    vsnprintf(builder->buffer + builder->length, extra_length + 1, format, format_args);

    builder->length += extra_length;
    builder->buffer[builder->length] = '\0';
}

void string_builder_append(
    struct String_Builder *builder,
    const char *format, ...
) {
    va_list format_args;
    va_start(format_args, format);
    string_builder_append_vargs(builder, format, format_args);
    va_end(format_args);
}

void string_builder_clear(struct String_Builder *builder) {
    builder->length    = 0;
    if (builder->buffer) {
        builder->buffer[0] = '\0';
    }
}

void string_builder_destroy(struct String_Builder *builder) {
    allocator_release(builder->allocator, builder->buffer);
    builder->buffer = NULL;
    builder->length = 0;
    builder->capacity = 0;
}

static char *EMPTY_CSTRING = "";

struct String_View string_builder_as_string(struct String_Builder *builder) {
    if (builder->buffer == NULL) {
        // @TODO: Better errors
        return (struct String_View) {
            .data   = EMPTY_CSTRING,
            .length = 0,
        };
    }

    return (struct String_View) {
        .data   = builder->buffer,
        .length = builder->length,
    };
}

struct String string_builder_to_string(struct String_Builder *builder, struct Allocator *allocator) {
    if (builder->buffer == NULL) {
        // @TODO: Better errors
        return (struct String) {
            .data   = EMPTY_CSTRING,
            .length = 0,
        };
    }

    // @TODO: My own memcpy
    char *cloned_buffer = allocator_allocate(allocator, builder->length + 1);
    memcpy(cloned_buffer, builder->buffer, builder->length + 1);

    return (struct String) {
        .data   = cloned_buffer,
        .length = builder->length,
    };
}

