#include "allocators.h"

#include <stddef.h>
#include <string.h>
#include <assert.h>

static const size_t default_alignment = alignof(size_t);

void *allocator_allocate_aligned(
    struct Allocator *allocator,
    size_t size, size_t alignment
) {
    void *address = NULL;

    if (allocator->allocate && size != 0) {
        address = allocator->allocate(allocator->data_context, size, alignment);
    }

    return address;
}

void *allocator_allocate(struct Allocator *allocator, size_t size) {
    return allocator_allocate_aligned(allocator, size, default_alignment);
}

void allocator_release(struct Allocator *allocator, void *address) {
    if (allocator->release && address != NULL) {
        allocator->release(allocator->data_context, address);
    }
}

void *clone(void *data, size_t size, struct Allocator *allocator) {
    void *address = allocator_allocate(allocator, size);
    memmove(address, data, size); // LibC TODO
    return address;
}

intptr_t align_backward(intptr_t address, intptr_t alignment) {
    intptr_t aligned_address = address & ~(alignment - 1);
    assert(aligned_address % alignment == 0);
    return aligned_address;
}

intptr_t align_forward(intptr_t address, intptr_t alignment) {
    return align_backward(address + (alignment - 1), alignment);
}
