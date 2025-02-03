#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include <stddef.h>
#include "stdlib/allocators.h"
#include "stdlib/array_list.h"
#include "stdlib/strings.h"

struct String_Builder {
    struct Array_List_Header header;
    char  *items;
};

struct String_Builder string_builder_create(struct Allocator *);
void string_builder_append       (struct String_Builder *, const char *, ...);
void string_builder_append_vargs (struct String_Builder *, const char *, va_list);
void string_builder_clear   (struct String_Builder *);
void string_builder_destroy (struct String_Builder *);

struct String_View string_builder_as_string(struct String_Builder *);
struct String      string_builder_to_string(struct String_Builder *, struct Allocator *);

#endif // STRING_BUILDER_H
