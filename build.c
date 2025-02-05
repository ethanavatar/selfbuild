#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include "self_build/self_build.h"
#include "self_build/self_build.c"

#include "stdlib/win32_platform.c"
#include "stdlib/strings.c"
#include "stdlib/allocators.c"
#include "stdlib/arena.c"
#include "stdlib/thread_context.c"
#include "stdlib/managed_arena.c"
#include "stdlib/scratch_memory.c"
#include "stdlib/string_builder.c"
#include "stdlib/list.c"
#include "stdlib/libc_allocator.c"

extern struct Build __declspec(dllexport) build(struct Build_Context *, struct Build_Options);

struct Build build(struct Build_Context *context, struct Build_Options options) {
    struct Allocator *allocator = &context->allocator;
    struct Build lib = build_create(context, options, "self_build");

    list_extend(&lib.sources, win32_list_files("stdlib",     "*.c", allocator));
    list_extend(&lib.sources, win32_list_files("self_build", "*.c", allocator));

    return lib;
}

struct Build build_tests(struct Build_Context *context, struct Build_Options options) {
    struct Build test_exe = build_create(context, options, "tests");
    list_extend(&test_exe.sources, win32_list_files("tests", "*.c", &context->allocator));
    return test_exe;
}

int main(void) {
    struct Thread_Context tctx;
    thread_context_init_and_equip(&tctx);

    struct Allocator scratch = scratch_begin();

    char *artifacts_directory = "bin";
    char *self_build_path     = ".";

    if (!win32_dir_exists(artifacts_directory)) win32_create_directories(artifacts_directory);
    char *cwd = win32_get_current_directory(&scratch);
    
    // @TODO: Figure out a way to check if the current run is a result of a bootstrapping
    // because if so, it should ignore any incremental build checks and build everything
    bootstrap("build.c", "build.exe", "bin/build.old", ".");

    struct Build_Context context = {
        // @Hack: This is patchwork for until I rewrite the scratch allocator to work properly
        // for hierarchical lifetimes
        .allocator = libc_allocator(),

        .current_directory   = cwd,
        .self_build_path     = self_build_path,
        .artifacts_directory = artifacts_directory,
        .debug_info_kind     = Debug_Info_Kind_Portable,
    };

    struct Build_Options module_options = { .build_kind = Build_Kind_Static_Library };
    struct Build module = build(&context, module_options);
    module.root_dir     = ".";

    struct Build_Options test_options = { .build_kind = Build_Kind_Executable };
    struct Build tests_exe = build_tests(&context, test_options);
    tests_exe.root_dir     = ".";

    add_dependency(&tests_exe, module);
    build_module(&context, &tests_exe);

    /*
    managed_arena_print((struct Managed_Arena *) scratch.data_context);
    fprintf(stderr, "\n");
    */

    scratch_end(&scratch);
    thread_context_release();
    return 0;
}
