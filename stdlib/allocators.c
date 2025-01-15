#include "allocators.h"

#include <stdio.h>
#include <assert.h>

void *allocator_allocate(struct Allocator *allocator, size_t size) {
    void *result = NULL;
    if (allocator->allocate && size != 0) {
        result = allocator->allocate(allocator->data_context, size);
    }
    return result;
}

void allocator_release(struct Allocator *allocator, void *address) {
    if (allocator->release && address != NULL) {
        allocator->release(allocator->data_context, address);
    }
}

