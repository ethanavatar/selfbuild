#include "libc_allocator.h"

#include <stddef.h>
#include "stdlib/allocators.h"

#include <stdlib.h>

#if defined(_WIN32)
#include <malloc.h>
#endif

static const struct Allocator value = {
    .data_context = NULL,
    .allocate     = libc_allocate,
    .release      = libc_release,
};

struct Allocator libc_allocator(void) { return value; }

inline static void *malloc_aligned(size_t size, size_t alignment) {
#if defined(_WIN32)
    return _aligned_malloc(size, alignment);
#else

    return aligned_alloc(alignment, size); 
#endif
}

inline static void free_aligned(void *address) {
#if defined(_WIN32)
    _aligned_free(address);
#else

    free(address); 
#endif
}

void *libc_allocate (void *, size_t size, size_t alignment) { return malloc_aligned(size, alignment); }
void  libc_release  (void *, void *address) { free_aligned(address); }

