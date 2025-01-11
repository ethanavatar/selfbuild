#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "string_builder.h"

#include "allocators.h"

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

void string_builder_append(
    struct String_Builder *builder,
    const char *format, ...
) {
    va_list args;
    va_start(args, format);
    size_t extra_length = vsnprintf(NULL, 0, format, args);
    va_end(args);

    bool needs_resize = builder->length + (extra_length + 1) > builder->capacity;

    if (needs_resize) {
        size_t new_capacity = builder->capacity == 0
            ? extra_length + 1
            : builder->capacity * 2;

        char *new_buffer = allocator_allocate(builder->allocator, new_capacity);

        if (builder->buffer) {
            // @TODO: My own memcpy
            memcpy(new_buffer, builder->buffer, builder->length);
            allocator_release(builder->allocator, builder->buffer);
        }

        builder->buffer = new_buffer;
        builder->capacity = new_capacity;
    }

    va_start(args, format);
    vsnprintf(builder->buffer + builder->length, extra_length + 1, format, args);
    va_end(args);

    builder->length += extra_length;
    builder->buffer[builder->length] = '\0';
}

void string_builder_clear(struct String_Builder *builder) {
    builder->length    = 0;
    builder->buffer[0] = '\0';
}

struct String_View string_builder_as_string(struct String_Builder *builder) {
    if (builder->buffer == NULL) {
        // @TODO: Better errors
        return (struct String_View) {
            .data   = NULL,
            .length = 0,
        };
    }

    return (struct String_View) {
        .data   = builder->buffer,
        .length = builder->length,
    };
}

void string_builder_destroy(struct String_Builder *builder) {
    allocator_release(builder->allocator, builder->buffer);
    builder->buffer = NULL;
    builder->length = 0;
    builder->capacity = 0;
}
