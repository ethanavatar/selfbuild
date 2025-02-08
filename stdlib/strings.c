#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "stdlib/strings.h"
#include "stdlib/allocators.h"

// @Note: Interesting read
// https://stackoverflow.com/questions/52537188/format-strings-safely-when-vsnprintf-is-not-available
char *format_cstring(struct Allocator *allocator, const char *format, ...) {
    char *result = NULL;
    va_list args = { 0 };
    va_start(args, format);

    int size = vsnprintf(NULL, 0, format, args);
    if (size > 0) {
        result = allocator_allocate(allocator, (size + 1) * sizeof(char));
        memset(result, 0, size + 1);
        vsnprintf(result, size + 1, format, args);
    }

    va_end(args);
    return result;
}

struct String cstring_to_string(char *cstring, struct Allocator *allocator) {
    size_t length = strlen(cstring); // @LibC @TODO
    return (struct String) {
        .data   = clone(cstring, sizeof(char) * length, allocator),
        .length = length,
    };
}

bool strings_are_equal(struct String s1, struct String s2) {
    bool are_equal = false;
    bool are_comparable = s1.length == s2.length;

    if (are_comparable) {
        are_equal = true;
        for (size_t i = 0; (i < s1.length) && are_equal; ++i) {
            are_equal = s1.data[i] == s2.data[i];
        }
    }

    return are_equal;
}
