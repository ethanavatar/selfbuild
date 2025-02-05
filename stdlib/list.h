#ifndef LIST_H_
#define LIST_H_

#include <stddef.h>
#include "stdlib/allocators.h"

struct List_Header {
    size_t count;
    size_t capacity;
    struct Allocator *allocator;
};

void _list_init   (struct List_Header *header, struct Allocator *allocator);
void _list_destroy(struct List_Header *header, void *items);

void _list_ensure_can_append(
    struct List_Header *header,
    void **items,
    size_t item_size,
    size_t items_to_add
);

void _list_clear(struct List_Header *header, void *items);
size_t _list_length(struct List_Header header);

#define list_init(collection, allocator) \
    (_list_init(&(collection)->header, (allocator)))

#define list_append(collection, item) do { \
        _list_ensure_can_append(      \
            &(collection)->header,          \
            (void **) &(collection)->items, \
            sizeof(*(collection)->items), 1 \
        );  \
        (collection)->items[(collection)->header.count++] = (item); \
    } while (0);

#define list_append_many(collection, items_to_add, items_count) do { \
        _list_ensure_can_append(      \
            &(collection)->header,          \
            (void **) &(collection)->items, \
            sizeof(*(collection)->items),   \
            items_count \
        ); \
        for ( \
            size_t _list_appended_count = 0; \
            _list_appended_count < items_count; \
            _list_appended_count++ \
        ) { \
            (collection)->items[(collection)->header.count++] = \
                (items_to_add)[_list_appended_count]; \
        } \
    } while (0);

#define list_extend(collection1, collection2) do { \
        _list_ensure_can_append(      \
            &(collection1)->header,          \
            (void **) &(collection1)->items, \
            sizeof(*(collection1)->items),   \
            list_length(collection2) \
        ); \
        for ( \
            size_t _list_appended_count = 0; \
            _list_appended_count < list_length(collection2); \
            _list_appended_count++ \
        ) { \
            (collection1)->items[(collection1)->header.count++] = \
                (collection2).items[_list_appended_count]; \
        } \
    } while (0);

#define list_destroy(collection) \
    (_list_destroy(&(collection)->header, (collection)->items))

#define list_clear(collection) \
    (_list_clear(&(collection)->header, (collection)->items))

#define list_length(collection) \
    (_list_length((collection).header))

#endif // LIST_H_
