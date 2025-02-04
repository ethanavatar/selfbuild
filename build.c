#include <stdio.h>
#include <stddef.h>
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

extern struct Build __declspec(dllexport) build(struct Build_Context *, enum Build_Kind);

struct Build build(struct Build_Context *context, enum Build_Kind requested_kind) {
    struct Build lib = build_create(context, requested_kind, "self_build");

    list_append(&lib.includes, cstring_to_string(".", &context->allocator));
    list_append(&lib.compile_flags, cstring_to_string("-g", &context->allocator));
    list_append(&lib.compile_flags, cstring_to_string("-gcodeview", &context->allocator));

    struct String_List stdlib_sources = win32_list_files("stdlib", "*.c", &context->allocator);

    for (size_t i = 0; i < list_length(stdlib_sources); ++i) {
        list_append(&lib.sources, stdlib_sources.items[i]);
    }

    // @TODO: list_extend, or list_concatenate

    struct String_List selfbuild_sources = win32_list_files("self_build", "*.c", &context->allocator);

    for (size_t i = 0; i < list_length(selfbuild_sources); ++i) {
        list_append(&lib.sources, selfbuild_sources.items[i]);
    }

    if (requested_kind == Build_Kind_Shared_Library) {
        list_append(&lib.link_flags, cstring_to_string("-lkernel32", &context->allocator));
        list_append(&lib.link_flags, cstring_to_string("-luser32",   &context->allocator));
        list_append(&lib.link_flags, cstring_to_string("-lshell32",  &context->allocator));
        list_append(&lib.link_flags, cstring_to_string("-lwinmm",    &context->allocator));
        list_append(&lib.link_flags, cstring_to_string("-lgdi32",    &context->allocator));
        list_append(&lib.link_flags, cstring_to_string("-lopengl32", &context->allocator));
    }

    return lib;
}

struct Build build_tests(struct Build_Context *context) {
    static struct Build test_exe = {
        .name = "tests",
        .kind = Build_Kind_Executable,
    };

    list_init(&test_exe.sources,       &context->allocator);
    list_init(&test_exe.includes,      &context->allocator);
    list_init(&test_exe.compile_flags, &context->allocator);
    list_init(&test_exe.link_flags,    &context->allocator);
    list_init(&test_exe.dependencies,    &context->allocator);

    list_append(&test_exe.sources, cstring_to_string("tests/tests_main.c", &context->allocator));

    list_append(&test_exe.includes, cstring_to_string(".", &context->allocator));

    list_append(&test_exe.compile_flags, cstring_to_string("-g", &context->allocator));
    list_append(&test_exe.compile_flags, cstring_to_string("-gcodeview", &context->allocator));

    list_append(&test_exe.link_flags, cstring_to_string("-g", &context->allocator));
    list_append(&test_exe.link_flags, cstring_to_string("-gcodeview", &context->allocator));
    list_append(&test_exe.link_flags, cstring_to_string("-Wl,--pdb=", &context->allocator));

    return test_exe;
}

/*
struct Build build_opengl_test(struct Build_Context *context) {
    static char *test_files[]    = { "test/test.c" };
    static char *test_includes[] = { ".", "cglm/include" };

    static char *compile_flags[] = {
        "-g", "-gcodeview",
    };

    static char *link_flags[] = {
        "-lgdi32", "-lopengl32", "-lwinmm",
        "-g", "-gcodeview", "-Wl,--pdb=",
        //"-fsanitize=address,undefined"
    };

    static struct Build test_exe = {
        .kind = Build_Kind_Executable,
        .name = "opengl_test",

        .sources             = test_files,
        .sources_count       = sizeof(test_files) / sizeof(char *),

        .compile_flags       = compile_flags,
        .compile_flags_count = sizeof(compile_flags) / sizeof(char *),

        .link_flags          = link_flags,
        .link_flags_count    = sizeof(link_flags) / sizeof(char *),

        .includes            = test_includes,
        .includes_count      = sizeof(test_includes) / sizeof(char *),
    };

    return test_exe;
}
*/

#include <stdlib.h>

void *libc_allocate (void *, size_t size)   { return calloc(1, size); }
void  libc_release  (void *, void *address) { free(address); }

static const struct Allocator libc_allocator = {
    .data_context = NULL,
    .allocate     = libc_allocate,
    .release      = libc_release,
};

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

#if 0
    struct Build module = build(&context, Build_Kind_Static_Library);
    module.root_dir = ".";

    struct Build opengl_test_exe = build_opengl_test(&context);
    opengl_test_exe.root_dir     = ".";
    opengl_test_exe.dependencies = calloc(1, sizeof(struct Build));
    add_dependency(&opengl_test_exe, module);
    build_module(&context, &opengl_test_exe);

    struct Build tests_exe = build_tests(&context);
    tests_exe.root_dir     = ".";
    tests_exe.dependencies = calloc(1, sizeof(struct Build));
    add_dependency(&tests_exe, module);
    build_module(&context, &tests_exe);
#else
    struct Build module = build(&context, Build_Kind_Static_Library);
    module.root_dir = ".";

    struct Build tests_exe = build_tests(&context);
    tests_exe.root_dir     = ".";
    add_dependency(&tests_exe, module);
    build_module(&context, &tests_exe);
#endif

    managed_arena_print((struct Managed_Arena *) scratch.data_context);
    fprintf(stderr, "\n");

    scratch_end(&scratch);
    thread_context_release();
    return 0;
}
