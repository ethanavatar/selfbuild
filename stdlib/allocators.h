#ifndef ALLOCATORS_H
#define ALLOCATORS_H

#include <stddef.h>
#include <stdint.h>

#define KiB(KIBS) (1024 * KIBS)
#define MiB(MIBS) (1024 * KiB(MIBS))

typedef void *(*Allocator_Allocate) (void *, size_t, size_t);
typedef void  (*Allocator_Release)  (void *, void *);

struct Allocator {
    void *data_context;
    Allocator_Allocate allocate;
    Allocator_Release  release;
};

void *allocator_allocate (struct Allocator *, size_t);
void *allocator_allocate_aligned (struct Allocator *, size_t, size_t);

void  allocator_release  (struct Allocator *, void *);


void    *clone          (void *data, size_t size, struct Allocator *allocator);
intptr_t align_backward (intptr_t address, intptr_t alignment);
intptr_t align_forward  (intptr_t address, intptr_t alignment);

#endif // ALLOCATORS_H
