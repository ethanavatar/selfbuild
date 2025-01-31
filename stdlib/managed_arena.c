#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "stdlib/win32_platform.h"
#include "stdlib/managed_arena.h"
#include "stdlib/memory.h"

#include <windows.h>

static const int COMMIT_SIZE  = KiB(64);
static const int RESERVE_SIZE = MiB(64);

struct Managed_Arena managed_arena_create(void) {
    struct Managed_Arena arena = { 0 };
    arena.memory = VirtualAlloc(NULL, RESERVE_SIZE, MEM_RESERVE, PAGE_READWRITE);
    VirtualAlloc(arena.memory, COMMIT_SIZE, MEM_COMMIT, PAGE_READWRITE);
    arena.committed_bytes = COMMIT_SIZE;
    arena.reserved_bytes  = RESERVE_SIZE;
    return arena;
}

void managed_arena_print(struct Managed_Arena *arena) {
    fprintf(stderr, "{\n");
    fprintf(stderr, "\t.memory           = %p,\n",  arena->memory);
    fprintf(stderr, "\t.used_bytes       = %zu,\n", arena->used_bytes);
    fprintf(stderr, "\t.reserved_bytes   = %zu,\n", arena->reserved_bytes);
    fprintf(stderr, "\t.committed_bytes  = %zu,\n", arena->committed_bytes);
    fprintf(stderr, "\t.high_water_bytes = %zu,\n", arena->high_water_bytes);
    fprintf(stderr, "}");
}

void *managed_arena_allocate(void *data_context, size_t size) {
    struct Managed_Arena *self = (struct Managed_Arena *) data_context;

    void *result = NULL;

    size_t new_used_bytes = self->used_bytes + size;
    if (new_used_bytes <= self->reserved_bytes) {
        result = (void *) (self->used_bytes + (ptrdiff_t) self->memory);
        self->used_bytes = new_used_bytes;

        if (self->used_bytes > self->committed_bytes) {
            intptr_t new_page = self->used_bytes + (intptr_t) self->memory;
            VirtualAlloc((void *) new_page, COMMIT_SIZE, MEM_COMMIT, PAGE_READWRITE);
            self->committed_bytes += COMMIT_SIZE;
        }

        if (self->used_bytes > self->high_water_bytes) {
            self->high_water_bytes = self->used_bytes;
        }

    } else {
        assert(false && "out of memory");
    }

    //managed_arena_print(self);
    return result;
}

void managed_arena_release(void *data_context, void *memory) { }
void managed_arena_destroy(void *data_context) {
    struct Managed_Arena *self = (struct Managed_Arena *) data_context;
    VirtualFree(self->memory, 0, MEM_RELEASE);
}


struct Allocator managed_arena_allocator(struct Managed_Arena *arena) {
    return (struct Allocator) {
        .data_context = arena,
        .allocate     = managed_arena_allocate,
        .release      = managed_arena_release,
    };
}
