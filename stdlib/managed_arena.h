#ifndef MANAGED_ARENA_H
#define MANAGED_ARENA_H

#include <stddef.h>
#include <stdint.h>

struct Managed_Arena {
    void  *memory;
    size_t used_bytes;

    size_t reserved_bytes;
    size_t committed_bytes;

    size_t high_water_bytes;

    size_t return_stack[255];
    size_t return_stack_count;
};

struct Managed_Arena managed_arena_create(void);

void *managed_arena_allocate(void *, size_t, size_t);
void managed_arena_release  (void *, void *);
void managed_arena_destroy  (void *);

struct Allocator managed_arena_allocator(struct Managed_Arena *);

#endif // MANAGED_ARENA_H
