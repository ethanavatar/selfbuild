#include "stdlib/list.h"
#include "stdlib/strings.h"

bool test_list_of_strings(struct Allocator *scratch) {
    struct String_List strings = { 0 };
    list_init(&strings, scratch);

    list_append(&strings, cstring_to_string("Hair", scratch));
    list_append(&strings, cstring_to_string("to",   scratch));
    list_append(&strings, cstring_to_string("hair", scratch));

    struct String expected[] = {
        cstring_to_string("Hair", scratch),
        cstring_to_string("to",   scratch),
        cstring_to_string("hair", scratch),
    };

    bool success = true;

    for (size_t i = 0; (i < list_length(strings)) && success; ++i) {
        struct String s = strings.items[i];
        success = strings_are_equal(s, expected[i]);
    }

    list_destroy(&strings);
    return success;
}

