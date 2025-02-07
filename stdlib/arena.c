#include "stdlib/arena.h"

#include <stdio.h>
#include <assert.h>

#include "stdlib/allocators.h"
#include "stdlib/memory.c"

struct Arena arena_create(unsigned char *memory, size_t capacity_bytes) {
    return (struct Arena) {
        .begin = memory,
        .capacity = capacity_bytes,
        .length = 0,
    };
}

struct Allocator arena_allocator(struct Arena *arena) {
    return (struct Allocator) {
        .data_context = arena,
        .allocate     = arena_allocate,
        .release      = arena_release,
    };
}

void arena_print(struct Arena *arena) {
    fprintf(stderr, "{\n");
    fprintf(stderr, "\t.begin    = %p,\n",  arena->begin);
    fprintf(stderr, "\t.capacity = %zu,\n", arena->capacity);
    fprintf(stderr, "\t.length   = %zu,\n", arena->length);
    fprintf(stderr, "}");
}

void *arena_allocate(void *data_context, size_t size, size_t alignment) {
    struct Arena *self = (struct Arena *) data_context;

    void *allocation = (void *) align_forward(self->length + (intptr_t) self->begin, alignment);
    ptrdiff_t padding = (intptr_t) allocation - ((intptr_t) self->begin + self->length);
    assert((self->length + padding + size) <= self->capacity && "Out of memory");

    self->length += padding + size;

    return allocation;
}

void arena_release(void *data_context, void *address) { }

void arena_clear(void *data_context) {
    struct Arena *self = (struct Arena *) data_context;
    self->length = 0;
}
