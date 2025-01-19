#ifndef ARENA_H
#define ARENA_H

#include "stdlib/allocators.h"

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

#endif // ARENA_H
