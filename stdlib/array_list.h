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
void _array_list_ensure_can_append(struct Array_List_Header *header, void **items, size_t item_size, size_t items_to_add);
void _array_list_destroy(struct Array_List_Header *header, void *items);

#define array_list_init(collection, allocator) \
    (_array_list_init(&(collection)->header, (allocator)))

#define array_list_append(collection, item) do { \
        _array_list_ensure_can_append(      \
            &(collection)->header,          \
            (void **) &(collection)->items, \
            sizeof(*(collection)->items), 1 \
        );  \
        (collection)->items[(collection)->header.count++] = (item); \
    } while (0);

#define array_list_append_many(collection, items_to_add, items_count) do { \
        _array_list_ensure_can_append(      \
            &(collection)->header,          \
            (void **) &(collection)->items, \
            sizeof(*(collection)->items),   \
            items_count \
        ); \
        for ( \
            size_t _array_list_appended_count = 0; \
            _array_list_appended_count < items_count; \
            _array_list_appended_count++ \
        ) { \
            (collection)->items[(collection)->header.count++] = (items_to_add)[_array_list_appended_count]; \
        } \
    } while (0);

#define array_list_destroy(collection) \
    (_array_list_destroy(&(collection)->header, (collection)->items))


#endif // ARRAY_LIST_H_
