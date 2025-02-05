#include "self_build/self_build.h"
#include "self_build/self_build.c"

#define ALL_STDLIB_C_
#include "self_build/all_stdlib.h"

extern struct Build __declspec(dllexport) build(struct Build_Context *, struct Build_Options);

struct Build build(struct Build_Context *context, struct Build_Options options) {
    struct Allocator *allocator = &context->allocator;
    struct Build lib = build_create(context, options, "self_build");

    list_extend(&lib.sources, win32_list_files("stdlib",     "*.c", allocator));
    list_extend(&lib.sources, win32_list_files("self_build", "*.c", allocator));

    return lib;
}

struct Build build_tests(struct Build_Context *context, struct Build_Options options) {
    struct Allocator *allocator = &context->allocator;
    struct Build test_exe = build_create(context, options, "tests");

    list_extend(&test_exe.sources, win32_list_files("tests", "*.c", allocator));

    return test_exe;
}

int main(void) {
    struct Thread_Context tctx = { 0 };
    thread_context_init_and_equip(&tctx);

    struct Allocator allocator = scratch_begin(NULL);

    char *artifacts_directory = "bin";
    char *self_build_path     = ".";

    if (!win32_dir_exists(artifacts_directory)) win32_create_directories(artifacts_directory);

    struct Build_Context context = {
        .allocator = allocator,
        .current_directory   = win32_get_current_directory(&allocator),
        .self_build_path     = self_build_path,
        .artifacts_directory = artifacts_directory,
        .debug_info_kind     = Debug_Info_Kind_Portable,
    };
    
    bootstrap(&context, "build.c", "build.exe", self_build_path);

    struct Build_Options module_options = { .build_kind = Build_Kind_Static_Library };
    struct Build module = build(&context, module_options);
    module.root_dir     = "."; // @TODO: Find a way to make this line unnecessary

    struct Build_Options test_options = { .build_kind = Build_Kind_Executable };
    struct Build tests_exe = build_tests(&context, test_options);
    tests_exe.root_dir     = ".";

    add_dependency(&tests_exe, module);
    build_module(&context, &tests_exe);

    scratch_end(&allocator);

    thread_context_release();
    return 0;
}
