#include <stdio.h>
#include <assert.h>
#include "memory.h"

void *allocator_allocate(struct Allocator *allocator, size_t size) {
    void *result = NULL;
    if (allocator->allocate) {
        result = allocator->allocate(allocator->data_context, size);
    }
    return result;
}

void allocator_release(struct Allocator *allocator, void *address) {
    if (allocator->release) {
        allocator->release(allocator->data_context, address);
    }
}

struct Arena arena_create(unsigned char *memory, size_t capacity_bytes) {
    return (struct Arena) {
        .memory = memory,
        .capacity_bytes   = capacity_bytes,
        .used_bytes       = 0,
        .high_water_bytes = 0,
    };
}

struct Allocator arena_allocator(struct Arena *arena) {
    return (struct Allocator) {
        .data_context = arena,
        .allocate = arena_allocate,
        .release  = arena_release,
    };
}

void arena_print(struct Arena *arena) {
    fprintf(stderr, "{\n");
    fprintf(stderr, "\t.memory           = %p,\n",  arena->memory);
    fprintf(stderr, "\t.capacity_bytes   = %zu,\n", arena->capacity_bytes);
    fprintf(stderr, "\t.used_bytes       = %zu,\n", arena->used_bytes);
    fprintf(stderr, "\t.high_water_bytes = %zu,\n", arena->high_water_bytes);
    fprintf(stderr, "}");
}

void *arena_allocate(void *data_context, size_t size) {
    struct Arena *self = (struct Arena *) data_context;

    void *result = NULL;

    size_t new_used_bytes = self->used_bytes + size;
    if (new_used_bytes <= self->capacity_bytes) {
        result = (void *) (self->used_bytes + (ptrdiff_t) self->memory);
        self->used_bytes = new_used_bytes;

        if (self->used_bytes > self->high_water_bytes) {
            self->high_water_bytes = self->used_bytes;
        }

    } else {
        assert(false && "out of memory");
    }

    return result;
}

void arena_release(void *data_context, void *address) { }

void arena_clear(void *data_context) {
    struct Arena *self = (struct Arena *) data_context;
    
    self->used_bytes = 0;
}
