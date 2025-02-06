#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "stdlib/thread_context.h"
#include "stdlib/win32_platform.h"

#include <windows.h>

#include "stdlib/managed_arena.h"

thread_local struct Thread_Context *thread_local_context = NULL;

void thread_context_init_and_equip(struct Thread_Context *context) {
    // @TODO: My own memset
    memset(context, 0, sizeof(struct Thread_Context));
    context->arenas[0] = managed_arena_create();
    context->arenas[1] = managed_arena_create();
    thread_local_context = context;
}

struct Thread_Context *thread_context_get(void) {
    return thread_local_context;
}

void thread_context_release(void) {
    managed_arena_destroy(&thread_local_context->arenas[0]);
    managed_arena_destroy(&thread_local_context->arenas[1]);
    memset(thread_local_context, 0, sizeof(struct Thread_Context));
    thread_local_context = NULL;
}

