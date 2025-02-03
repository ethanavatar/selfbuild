#include "stdlib/allocators.h"
#include "stdlib/thread_context.h"
#include "stdlib/scratch_memory.h"

#include "tests/test_memory.c"
#include "tests/test_array_list.c"

#define _STRINGIFY_IMPL(X) #X
#define STRINGIFY(X) _STRINGIFY_IMPL(X)

#define TEST_FUNCTIONS \
    X(test_clone)     \
    X(test_array_list_of_characters) \
    X(test_array_list_of_strings)

typedef bool (*Test_Function) (struct Allocator *test_allocator);

#define X(function) function,
static Test_Function tests_functions[] = { TEST_FUNCTIONS };
#undef X

#define X(function) STRINGIFY(function),
static const char   *tests_names[]     = { TEST_FUNCTIONS };
#undef X


int main(void) {
    struct Thread_Context tctx;
    thread_context_init_and_equip(&tctx);

    size_t tests_count = sizeof(tests_functions) / sizeof(Test_Function);
    bool tests_results[tests_count];

    for (size_t tests_index = 0; tests_index < tests_count; ++tests_index) {
        struct Allocator scratch = scratch_begin();
        tests_results[tests_index] = tests_functions[tests_index](&scratch);
        scratch_end(&scratch);

        const char *pass_fail = tests_results[tests_index] ? "pass" : "fail";
        fprintf(stderr, "%s: %s\n", tests_names[tests_index], pass_fail);
    }

    thread_context_release();
}
