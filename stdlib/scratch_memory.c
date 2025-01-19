#include <stdio.h>

#include "stdlib/scratch_memory.h"
#include "stdlib/thread_context.h"
#include "stdlib/managed_arena.h"

struct Allocator scratch_begin(void) {
    struct Thread_Context *tctx  = thread_context_get();
    struct Managed_Arena  *arena = &tctx->arena;
    arena->return_stack[arena->return_stack_count++] = arena->used_bytes;
    //fprintf(stderr, "snapshot at %zu\n", arena->used_bytes);
    return managed_arena_allocator(&tctx->arena);
}

void scratch_end(struct Allocator *allocator) {
    struct Managed_Arena *arena = (struct Managed_Arena *) allocator->data_context;
    arena->used_bytes = arena->return_stack[--arena->return_stack_count];
    //fprintf(stderr, "restore to %zu\n", arena->used_bytes);
}
