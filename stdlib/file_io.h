#ifndef FILE_IO_H
#define FILE_IO_H

#include <stddef.h>
#include "stdlib/allocators.h"

struct File_Contents {
    bool   is_valid;
    size_t length;
    char  *contents;
};

struct File_Contents file_read_to_end(const char *, struct Allocator *);

#endif // FILE_IO_H
