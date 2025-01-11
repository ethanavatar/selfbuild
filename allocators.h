#ifndef ALLOCATORS_H
#define ALLOCATORS_H

#include <stddef.h>

typedef void *(*Allocator_Allocate) (void *, size_t);
typedef void  (*Allocator_Release)  (void *, void *);

struct Allocator {
    void *data_context;
    Allocator_Allocate allocate;
    Allocator_Release  release;
};

void *allocator_allocate (struct Allocator *, size_t);
void  allocator_release  (struct Allocator *, void *);

#endif // ALLOCATORS_H
