#include <stdio.h>
#include <assert.h>
#include "allocators.h"

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

