#include "self_build/self_build.h"
#include "self_build/self_build.c"

#define ALL_STDLIB_C_
#include "self_build/all_stdlib.h"

extern struct Build __declspec(dllexport) build(struct Build_Context *, enum Build_Kind);

struct Build build(struct Build_Context *context, enum Build_Kind requested_kind) {
    struct Allocator *allocator = &context->allocator;
    struct Build lib = build_create(context, requested_kind, "self_build");

    list_extend(&lib.sources, win32_list_files("stdlib",     "*.c", allocator));
    list_extend(&lib.sources, win32_list_files("self_build", "*.c", allocator));

    return lib;
}

struct Build build_tests(struct Build_Context *context, enum Build_Kind requested_kind) {
    struct Allocator *allocator = &context->allocator;
    struct Build test_exe = build_create(context, requested_kind, "tests");

    list_extend(&test_exe.sources, win32_list_files("tests", "*.c", allocator));

    return test_exe;
}

static struct Build_Context_Options options = {
    .debug_info_kind = Debug_Info_Kind_Portable,
};

int main(void) {
    struct Thread_Context tctx = { 0 };
    thread_context_init_and_equip(&tctx);

    struct Allocator allocator = scratch_begin(NULL);
    struct Build_Context context = build_create_context(options, ".", "bin", &allocator);
    
    bootstrap(&context, "build.c", "build.exe");

    struct Build module    = build(&context, Build_Kind_Static_Library);
    struct Build tests_exe = build_tests(&context, Build_Kind_Executable);

    add_dependency(&tests_exe, module);
    build_module(&context, &tests_exe);

    scratch_end(&allocator);
    thread_context_release();
    return 0;
}
