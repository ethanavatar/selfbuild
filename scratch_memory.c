#include "scratch_memory.h"

#include "thread_context.h"
#include "managed_arena.h"

struct Allocator scratch_begin(void) {
    struct Thread_Context *tctx = thread_context_get();
    return managed_arena_allocator(&tctx->arena);
}

void scratch_end(struct Allocator *allocator) {
    struct Managed_Arena *arena = (struct Managed_Arena *) allocator->data_context;
    arena->used_bytes = 0;
}
