#ifndef STRINGS_H
#define STRINGS_H

#include "stdlib/allocators.h"

struct String_View { char *data; size_t length; };
struct String      { char *data; size_t length; };

char *format_cstring(struct Allocator *, const char *, ...);

struct String cstring_to_string(char *cstring, struct Allocator *allocator);

#endif // STRINGS_H
