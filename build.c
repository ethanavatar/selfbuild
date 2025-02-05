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

void build_link_libraries(struct Build *build, struct Allocator *allocator, char *library) {
    list_append(&build->link_flags, cstring_to_string(lib_name, allocator));
}

extern struct Build __declspec(dllexport) build(struct Build_Context *, enum Build_Kind);

struct Build build(struct Build_Context *context, enum Build_Kind requested_kind) {
    struct Allocator *allocator = &context->allocator;

    struct Build lib = build_create(context, requested_kind, "self_build");

    list_append(&lib.includes, cstring_to_string(".", allocator));
    list_append(&lib.compile_flags, cstring_to_string("-g", allocator));
    list_append(&lib.compile_flags, cstring_to_string("-gcodeview", allocator));

    lib.sources = win32_list_files("stdlib", "*.c", &context->allocator);
    list_extend(&lib.sources, win32_list_files("self_build", "*.c", allocator));

    if (requested_kind == Build_Kind_Shared_Library) {
        list_append(&lib.link_flags, cstring_to_string("-lkernel32", allocator));
        list_append(&lib.link_flags, cstring_to_string("-luser32",   allocator));
        list_append(&lib.link_flags, cstring_to_string("-lshell32",  allocator));
        list_append(&lib.link_flags, cstring_to_string("-lwinmm",    allocator));
        list_append(&lib.link_flags, cstring_to_string("-lgdi32",    allocator));
        list_append(&lib.link_flags, cstring_to_string("-lopengl32", allocator));
    }

    return lib;
}

struct Build build_tests(struct Build_Context *context) {
    struct Build test_exe = build_create(context, Build_Kind_Executable, "tests");

    test_exe.sources = win32_list_files("tests", "*.c", &context->allocator);

    list_append(&test_exe.includes, cstring_to_string(".", &context->allocator));

    list_append(&test_exe.compile_flags, cstring_to_string("-g", &context->allocator));
    list_append(&test_exe.compile_flags, cstring_to_string("-gcodeview", &context->allocator));

    list_append(&test_exe.link_flags, cstring_to_string("-g", &context->allocator));
    list_append(&test_exe.link_flags, cstring_to_string("-gcodeview", &context->allocator));
    list_append(&test_exe.link_flags, cstring_to_string("-Wl,--pdb=", &context->allocator));

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
    };

    struct Build module = build(&context, Build_Kind_Static_Library);
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
