#ifndef ARRAY_LIST_H_
#define ARRAY_LIST_H_

#include <stddef.h>
#include "stdlib/allocators.h"

struct Array_List_Header {
    size_t count;
    size_t capacity;
    struct Allocator *allocator;
};

void _array_list_init   (struct Array_List_Header *header, struct Allocator *allocator);
void _array_list_ensure_can_append(struct Array_List_Header *header, void *items, size_t item_size);
void _array_list_destroy(struct Array_List_Header *header, void *items);

#define array_list_init(collection, allocator) \
    (_array_list_init(&(collection)->header, (allocator)))

#define array_list_append(collection, item) do { \
        _array_list_ensure_can_append(&(collection)->header, (collection)->items, sizeof(*(collection)->items)); \
        (collection)->items[(&(collection)->header)->count++] = (item); \
    } while (0);

#define array_list_destroy(collection) \
    (_array_list_destroy(&(collection)->header, (collection)->items))


#endif // ARRAY_LIST_H_
