#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "stdlib/array_list.h"
#include "stdlib/array_list.c"

struct Character_Array {
    struct Array_List_Header header;
    char *items;
};

#define append_cstring(collection, cstring) \
    array_list_append_many(collection, cstring, strlen(cstring))

bool test_array_list(struct Allocator *scratch) {
    struct Character_Array array = { 0 };
    array_list_init(&array, scratch);

    append_cstring(&array, "Six to one");
    array_list_append(&array, ',');
    array_list_append(&array, ' ');

    append_cstring(&array, "Sister.");
    array_list_append(&array, '\n');
    array_list_append(&array, '\0');

    const char *expected = "Six to one, Sister.\n";
    bool result = strncmp(array.items, expected, array.header.count) == 0;

    array_list_destroy(&array);
    return result;
}
