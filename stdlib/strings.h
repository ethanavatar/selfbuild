#ifndef STRINGS_H
#define STRINGS_H

#include <stdarg.h>
#include "stdlib/allocators.h"
#include "stdlib/list.h"

struct String_View { char *data; size_t length; };
struct String      { char *data; size_t length; };

char *format_cstring(struct Allocator *, const char *, ...);

struct String cstring_to_string(char *cstring, struct Allocator *allocator);

struct String_List {
    struct List_Header header;
    struct String *items;
};

/*
struct String string_list_join(
    struct String_List list, struct Allocator *allocator,
    const char *format
) {
    struct String_Builder sb = string_builder_create(allocator);

    for (size_t i = 0; i < list_length(list); ++i) {
        struct String s = list.items[i];
        string_builder_append(&sb, format, (int) s.length, s.data);
    }

    struct String_View view = string_builder_as_string(sb);

    // @Hack
    // This seems like a leaky abstraction.
    // The intent for there being a distinction between String and String_View is so indicate when you are responsible for
    // cleaning up the result.
    // A returned String is 'owned' by you, so you should free it when you're done.
    // A returned String_View is not your string, so you dont need to do anything with it.
    // In this case, the String should be owned because it was built in the lifetime of the provided allocator,
    // but because it is a view into the data of a String_Builder, its technically borrowed from local ownership.
    // There is no generalized way to conceptually promote borrowed data into owned data without cloning.
    // Maybe there shouldn't even be a difference between String and String_View because I can give the function whatever allocator
    // I want and it has no idea what the actual lifetime of it is going to be.
    return (struct String) {
        .data   = view.data,
        .length = view.length,
    };
}
*/

#endif // STRINGS_H
