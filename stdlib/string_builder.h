#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include <stddef.h>
#include "stdlib/allocators.h"

struct String_View { char *data; size_t length; };
struct String      { char *data; size_t length; };

struct String_Builder {
    struct Allocator *allocator;
    char  *buffer;
    size_t capacity;
    size_t length;
};

struct String_Builder string_builder_create(struct Allocator *, size_t);
void string_builder_append       (struct String_Builder *, const char *, ...);
void string_builder_append_vargs (struct String_Builder *, const char *, va_list);
void string_builder_clear   (struct String_Builder *);
void string_builder_destroy (struct String_Builder *);

struct String_View string_builder_as_string(struct String_Builder *);
struct String      string_builder_to_string(struct String_Builder *, struct Allocator *);

#endif // STRING_BUILDER_H
