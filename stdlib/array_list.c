#include <stddef.h>

#include "stdlib/array_list.h"
#include "stdlib/allocators.h"

void _array_list_init(struct Array_List_Header *header, struct Allocator *allocator) {
    *header = (struct Array_List_Header) {
        .count     = 0,
        .capacity  = 0,
        .allocator = allocator,
    };
}

static void _array_list_expand(
    struct Array_List_Header *header, void **items,
    size_t item_size
) {
    size_t old_size = header->capacity * item_size;

    header->capacity *= 2;

    if (header->capacity == 0) {
        header->capacity = 256;
    }

    void *new_items = allocator_allocate(header->allocator, header->capacity * item_size);
    assert(new_items != NULL && "Failed to resize the array list");

    if (old_size > 0) {
        memcpy(new_items, *items, old_size); // @LibC
        allocator_release(header->allocator, *items);
    }

    *items = new_items;
}

void _array_list_ensure_can_append(
    struct Array_List_Header *header, void **items,
    size_t item_size, size_t items_to_add
) {
    size_t new_count = header->count + items_to_add;
    while (new_count >= header->capacity) {
        _array_list_expand(header, items, item_size);
    }
}

void _array_list_destroy(struct Array_List_Header *header, void *items) {
    allocator_release(header->allocator, items);
    _array_list_init(header, header->allocator);
}

