#ifndef THREAD_CONTEXT_H
#define THREAD_CONTEXT_H

#include "stdlib/managed_arena.h"

struct Thread_Context {
    struct Managed_Arena arena;
};

void thread_context_init_and_equip(struct Thread_Context *);
void thread_context_release(void);

struct Thread_Context *thread_context_get(void);

#endif // THREAD_CONTEXT_H
