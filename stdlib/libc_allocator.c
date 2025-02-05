#include "libc_allocator.h"

#include <stddef.h>
#include "stdlib/allocators.h"

#include <stdlib.h>

static const struct Allocator value = {
    .data_context = NULL,
    .allocate     = libc_allocate,
    .release      = libc_release,
};

struct Allocator libc_allocator(void) { return value; }

void *libc_allocate (void *, size_t size)   { return calloc(1, size); }
void  libc_release  (void *, void *address) { free(address);          }

