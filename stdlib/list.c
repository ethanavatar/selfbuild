#include <stddef.h>
#include <assert.h>
#include <string.h>

#include "stdlib/list.h"
#include "stdlib/allocators.h"

void _list_init(struct List_Header *header, struct Allocator *allocator) {
    *header = (struct List_Header) {
        .count     = 0,
        .capacity  = 0,
        .allocator = allocator,
    };
}

static void _list_expand(
    struct List_Header *header, void **items,
    size_t item_size
) {
    size_t old_size = header->capacity * item_size;

    header->capacity *= 2;

    if (header->capacity == 0) {
        header->capacity = 256;
    }

    void *new_items = allocator_allocate(header->allocator, header->capacity * item_size);
    assert(new_items != NULL && "Failed to resize the list");

    if (old_size > 0) {
        // @TODO: My own memmove
        memmove(new_items, *items, old_size); // @LibC
        allocator_release(header->allocator, *items);
    }

    *items = new_items;
}

void _list_ensure_can_append(
    struct List_Header *header, void **items,
    size_t item_size, size_t items_to_add
) {
    size_t new_count = header->count + items_to_add;
    while (new_count >= header->capacity) {
        _list_expand(header, items, item_size);
    }
}

void _list_destroy(struct List_Header *header, void *items) {
    allocator_release(header->allocator, items);
    _list_init(header, header->allocator);
}

void _list_clear(struct List_Header *header, void *items) {
    header->count = 0;
}

size_t _list_length(struct List_Header *header) {
    return header->count;
}
