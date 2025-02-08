#include "stdlib/list.h"
#include "stdlib/strings.h"

bool test_strings_are_equal(struct Allocator *scratch) {
    // @TODO: These strings dont really need to be allocated.
    //        Make a kind of cstring_to_string with "trust me bro" lifetimes
    struct String s1 = cstring_to_string("Hekki Allmo", scratch);
    struct String s2 = cstring_to_string("Hekki Allmo", scratch);

    bool equal = strings_are_equal(s1, s2);

    struct String s3 = cstring_to_string("Hello Allmo", scratch);
    struct String s4 = cstring_to_string("Hekki Allmo", scratch);

    bool not_equal = !strings_are_equal(s3, s4);

    return equal && not_equal;
}
