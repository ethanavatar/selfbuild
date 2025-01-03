#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "strings.h"

// @Note: Interesting read
// https://stackoverflow.com/questions/52537188/format-strings-safely-when-vsnprintf-is-not-available
char *format_cstring(const char *format, ...) {
    char *result = NULL;
    va_list args;
    va_start(args, format);

    int size = vsnprintf(NULL, 0, format, args);
    if (size > 0) {
        result = calloc(size + 1, sizeof(char));
        memset(result, 0, size + 1);
        vsnprintf(result, size + 1, format, args);
    }

    va_end(args);
    return result;
}
