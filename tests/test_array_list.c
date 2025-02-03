#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "stdlib/array_list.h"

struct Character_Array {
    struct Array_List_Header header;
    char *items;
};

#define append_cstring(collection, cstring) \
    array_list_append_many(collection, cstring, strlen(cstring)) // @LibC @TODO

bool test_array_list_of_characters(struct Allocator *scratch) {
    struct Character_Array array = { 0 };
    array_list_init(&array, scratch);

    append_cstring(&array, "Six to one");
    array_list_append(&array, '.');
    array_list_append(&array, '\n');
    array_list_append(&array, '\0');

    const char *expected = "Six to one.\n";
    bool result = strncmp(array.items, expected, array.header.count) == 0; // @LibC @TODO

    array_list_destroy(&array);
    return result;
}

#include "stdlib/strings.h"

struct String_Array {
    struct Array_List_Header header;
    struct String *items;
};

bool test_array_list_of_strings(struct Allocator *scratch) {
    struct String_Array array = { 0 };
    array_list_init(&array, scratch);

    array_list_append(&array, cstring_to_string("Hair", scratch));
    array_list_append(&array, cstring_to_string("to",   scratch));
    array_list_append(&array, cstring_to_string("hair", scratch));

    const char *expected[] = {
        "Hair", "to", "hair",
    };

    bool success = true;

    for (size_t i = 0; i < array.header.count; ++i) {
        struct String s = array.items[i];
        //fprintf(stderr, "%zu: %.*s\n", i, (int) s.length, s.data);

        if (success) {
            success = strncmp(s.data, expected[i], s.length) == 0; // @LibC @TODO
        }
    }

    array_list_destroy(&array);
    return success;
}
