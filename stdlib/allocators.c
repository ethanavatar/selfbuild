#include "allocators.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

void *allocator_allocate(struct Allocator *allocator, size_t size) {
    void *address = NULL;
    if (allocator->allocate && size != 0) {
        address = allocator->allocate(allocator->data_context, size);
    }
    return address;
}

void allocator_release(struct Allocator *allocator, void *address) {
    if (allocator->release && address != NULL) {
        allocator->release(allocator->data_context, address);
    }
}

void *clone(void *data, size_t size, struct Allocator *allocator) {
    void *address = allocator_allocate(allocator, size);
    memmove(address, data, size);
    return address;
}

