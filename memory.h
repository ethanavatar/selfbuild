#ifndef MEMORY_H
#define MEMORY_H

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

struct Arena {
    unsigned char *memory;
    size_t used_bytes;
    size_t capacity_bytes;

    size_t high_water_bytes;
};

struct Arena     arena_create    (unsigned char *, size_t);
struct Allocator arena_allocator (struct Arena *);

void arena_print(struct Arena *);

void *arena_allocate (void *, size_t);
void  arena_release  (void *, void *);
void  arena_clear    (void *);

#endif // MEMORY_H

#ifdef MEMORY_C
#undef MEMORY_C
#include "memory.c"
#endif // MEMORY_C
