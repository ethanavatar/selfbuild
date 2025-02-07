#ifndef ARENA_H
#define ARENA_H

#include "stdlib/allocators.h"

struct Arena {
    unsigned char *begin;
    size_t length;
    size_t capacity;
};

struct Arena     arena_create    (unsigned char *, size_t);
struct Allocator arena_allocator (struct Arena *);

void arena_print(struct Arena *);

void *arena_allocate (void *, size_t, size_t);
void  arena_release  (void *, void *);
void  arena_clear    (void *);

#endif // ARENA_H
