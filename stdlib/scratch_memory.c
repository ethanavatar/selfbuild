#include <stdio.h>
#include <assert.h>

#include "stdlib/scratch_memory.h"
#include "stdlib/thread_context.h"
#include "stdlib/managed_arena.h"

struct Allocator scratch_begin(struct Allocator *conflict) {
    struct Thread_Context *tctx  = thread_context_get();
    
    if (tctx == NULL) {
        // @TODO: Real error reporting
        assert(false && "Thread context is null");
    }

    struct Managed_Arena *arena = NULL;
    if (conflict != NULL) {
        size_t i = 0;
        do {
            arena = &tctx->arenas[i++];
        } while (conflict->data_context == arena);

    } else {
        arena = &tctx->arenas[0];
    }

    arena->return_stack[arena->return_stack_count++] = arena->used_bytes;

    //fprintf(stderr, "snapshot at %zu\n", arena->used_bytes);
    return managed_arena_allocator(arena);
}

void scratch_end(struct Allocator *allocator) {
    struct Managed_Arena *arena = (struct Managed_Arena *) allocator->data_context;
    arena->used_bytes = arena->return_stack[--arena->return_stack_count];
    //fprintf(stderr, "restore to %zu\n", arena->used_bytes);
}
