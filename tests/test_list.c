#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "stdlib/list.h"

struct Character_List {
    struct List_Header header;
    char *items;
};

#define append_cstring(collection, cstring) \
    list_append_many(collection, cstring, strlen(cstring)) // @LibC @TODO

bool test_list_of_characters(struct Allocator *scratch) {
    struct Character_List characters = { 0 };
    list_init(&characters, scratch);

    append_cstring(&characters, "Six to one");
    list_append(&characters, '.');
    list_append(&characters, '\n');
    list_append(&characters, '\0');

    const char *expected = "Six to one.\n";
    bool result = strncmp(characters.items, expected, list_length(characters)) == 0; // @LibC @TODO

    list_destroy(&characters);
    return result;
}

#include "stdlib/strings.h"

bool test_list_of_strings(struct Allocator *scratch) {
    struct String_List strings = { 0 };
    list_init(&strings, scratch);

    list_append(&strings, cstring_to_string("Hair", scratch));
    list_append(&strings, cstring_to_string("to",   scratch));
    list_append(&strings, cstring_to_string("hair", scratch));

    const char *expected[] = {
        "Hair", "to", "hair",
    };

    bool success = true;

    for (size_t i = 0; i < list_length(strings); ++i) {
        struct String s = strings.items[i];
        //fprintf(stderr, "%zu: %.*s\n", i, (int) s.length, s.data);

        if (success) {
            success = strncmp(s.data, expected[i], s.length) == 0; // @LibC @TODO
        }
    }

    list_destroy(&strings);
    return success;
}
