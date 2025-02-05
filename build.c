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

void *libc_allocate (void *, size_t size)   { return calloc(1, size); }
void  libc_release  (void *, void *address) { free(address); }

static const struct Allocator libc_allocator = {
    .data_context = NULL,
    .allocate     = libc_allocate,
    .release      = libc_release,
};

extern struct Build __declspec(dllexport) build(struct Build_Context *, struct Build_Options);

struct Build build(struct Build_Context *context, struct Build_Options options) {
    struct Allocator *allocator = &context->allocator;
    struct Build lib = build_create(context, options, "self_build");

    list_append(&lib.includes, cstring_to_string(".", allocator));

    lib.sources = win32_list_files("stdlib", "*.c", &context->allocator);
    list_extend(&lib.sources, win32_list_files("self_build", "*.c", allocator));

    build_add_system_library(&lib, "kernel32");
    build_add_system_library(&lib, "user32");
    build_add_system_library(&lib, "shell32");
    build_add_system_library(&lib, "winmm");
    build_add_system_library(&lib, "gdi32");
    build_add_system_library(&lib, "opengl32");

    return lib;
}

struct Build build_tests(struct Build_Context *context) {
    struct Build_Options options = { .build_kind = Build_Kind_Executable };
    struct Build test_exe = build_create(context, options, "tests");

    test_exe.sources = win32_list_files("tests", "*.c", &context->allocator);
    list_append(&test_exe.includes, cstring_to_string(".", &context->allocator));

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
    
    bootstrap("build.c", "build.exe", "bin/build.old", ".");

    struct Build_Context context = {
        .self_build_path     = self_build_path,
        .artifacts_directory = artifacts_directory,
        .current_directory   = cwd,

        // @Hack: This is patchwork for until I rewrite the scratch allocator to work properly
        // for hierarchical lifetimes
        .allocator = libc_allocator,

        .debug_info_kind = Debug_Info_Kind_Portable,
    };

    struct Build_Options module_options = { .build_kind = Build_Kind_Static_Library };
    struct Build module = build(&context, module_options);
    module.root_dir = ".";

    struct Build tests_exe = build_tests(&context);
    tests_exe.root_dir     = ".";
    add_dependency(&tests_exe, module);
    build_module(&context, &tests_exe);

    managed_arena_print((struct Managed_Arena *) scratch.data_context);
    fprintf(stderr, "\n");

    scratch_end(&scratch);
    thread_context_release();
    return 0;
}
