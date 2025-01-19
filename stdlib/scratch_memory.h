#ifndef SCRATCH_MEMORY_H
#define SCRATCH_MEMORY_H

#include "stdlib/allocators.h"

struct Allocator scratch_begin(void);
void scratch_end(struct Allocator *);

#endif // SCRATCH_MEMORY_H
