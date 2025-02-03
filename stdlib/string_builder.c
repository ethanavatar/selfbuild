#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "stdlib/string_builder.h"
#include "stdlib/allocators.h"

struct String_Builder string_builder_create(
    struct Allocator *allocator
) {
    struct String_Builder b = { 0 };
    list_init(&b, allocator);
    return b;
}

void string_builder_append_vargs(
    struct String_Builder *builder,
    const char *format, va_list format_args
) {
    size_t extra_length = vsnprintf(NULL, 0, format, format_args);
    size_t item_size    = sizeof(*builder->items);

    _list_ensure_can_append(
        &builder->header, (void **) &builder->items,
        item_size, extra_length + 1
    );

    size_t write_offset = builder->header.count * item_size;
    vsnprintf(builder->items + write_offset, extra_length + 1, format, format_args);

    builder->header.count += extra_length;
    builder->items[builder->header.count] = '\0';
}

void string_builder_append(
    struct String_Builder *builder,
    const char *format, ...
) {
    va_list format_args = { 0 };
    va_start(format_args, format);
    string_builder_append_vargs(builder, format, format_args);
    va_end(format_args);
}

void string_builder_clear(struct String_Builder *builder) {
    list_clear(builder);
    if (builder->items) {
        builder->items[0] = '\0';
    }
}

void string_builder_destroy(struct String_Builder *builder) {
    list_destroy(builder);
}

static char *EMPTY_CSTRING = "";

struct String_View string_builder_as_string(struct String_Builder *builder) {
    if (builder->items == NULL) {
        // @TODO: Better errors
        return (struct String_View) {
            .data   = EMPTY_CSTRING,
            .length = 0,
        };
    }

    return (struct String_View) {
        .data   = builder->items,
        .length = builder->header.count,
    };
}

struct String string_builder_to_string(struct String_Builder *builder, struct Allocator *allocator) {
    if (builder->items == NULL) {
        // @TODO: Better errors
        return (struct String) {
            .data   = EMPTY_CSTRING,
            .length = 0,
        };
    }

    // @TODO: My own memcpy
    char *cloned_buffer = allocator_allocate(allocator, builder->header.count + 1);
    memcpy(cloned_buffer, builder->items, builder->header.count + 1);

    return (struct String) {
        .data   = cloned_buffer,
        .length = builder->header.count,
    };
}

