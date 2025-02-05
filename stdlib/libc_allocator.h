#ifndef LIBC_ALLOCATOR_H_
#define LIBC_ALLOCATOR_H_

#include <stddef.h>
#include "stdlib/allocators.h"

struct Allocator libc_allocator(void);

void *libc_allocate (void *, size_t size);
void  libc_release  (void *, void *address);

#endif // LIBC_ALLOCATOR_H_
