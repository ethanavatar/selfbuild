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

extern struct Build __declspec(dllexport) build(struct Build_Context *);

struct Build build(struct Build_Context *context) {

    static char *files[] = {
        "self_build/self_build.c",
        "stdlib/win32_platform.c",
        "stdlib/strings.c",

        "stdlib/allocators.c",
        "stdlib/arena.c", "stdlib/managed_arena.c",
        "stdlib/thread_context.c",
        "stdlib/scratch_memory.c",
        "stdlib/string_builder.c",
        "stdlib/file_io_win32.c",

        "windowing/windowing_win32.c",
        "windowing/drawing.c",
    };

    static char *includes[] = { "." };

    static char *compile_flags[] = {
        "-g", "-gcodeview",
    };

    static struct Build lib = {
        .kind = Build_Kind_Static_Library,
        .name = "self_build",

        .sources        = files,
        .sources_count  = sizeof(files) / sizeof(char *),

        .compile_flags       = compile_flags,
        .compile_flags_count = sizeof(compile_flags) / sizeof(char *),

        .includes       = includes,
        .includes_count = sizeof(includes) / sizeof(char *),
    };

    return lib;
}

struct Build test(struct Build_Context *context) {
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
        .name = "test",

        .sources        = test_files,
        .sources_count  = sizeof(test_files) / sizeof(char *),

        .compile_flags       = compile_flags,
        .compile_flags_count = sizeof(compile_flags) / sizeof(char *),

        .link_flags       = link_flags,
        .link_flags_count = sizeof(link_flags) / sizeof(char *),

        .includes       = test_includes,
        .includes_count = sizeof(test_includes) / sizeof(char *),
    };

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
    };

    struct Build module = build(&context);
    module.root_dir = ".";

    struct Build test_exe = test(&context);
    test_exe.root_dir = ".";
    test_exe.dependencies = calloc(1, sizeof(struct Build));
    add_dependency(&test_exe, module);

    build_module(&context, &test_exe);

    managed_arena_print((struct Managed_Arena *) scratch.data_context);
    fprintf(stderr, "\n");

    scratch_end(&scratch);
    thread_context_release();
    return 0;
}
