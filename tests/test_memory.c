#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "stdlib/allocators.h"

bool test_clone(struct Allocator *scratch) {
    char *s1 = "Sphere to square.";

    size_t s1_length = strlen(s1); // @LibC @TODO
    char *s2 = clone(s1, sizeof(char) * s1_length, scratch);

    bool strings_are_equal   = strncmp(s1, s2, s1_length) == 0; // @LibC @TODO
    bool pointers_are_unique = ((intptr_t) s1) != ((intptr_t) s2);

    return strings_are_equal && pointers_are_unique;
}
