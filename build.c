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

    static char *main_files[] = {
        "self_build/self_build.c",
        "stdlib/win32_platform.c",
        "stdlib/strings.c",

        "stdlib/allocators.c",
        "stdlib/arena.c", "stdlib/managed_arena.c",
        "stdlib/thread_context.c",
        "stdlib/scratch_memory.c",
        "stdlib/string_builder.c",
    };

    static char *main_includes[] = { "." };

    static struct Build exe = {
        .kind = Build_Kind_Module,
        .name = "self_build",

        .sources        = main_files,
        .sources_count  = sizeof(main_files) / sizeof(char *),

        .includes       = main_includes,
        .includes_count = sizeof(main_includes) / sizeof(char *),
    };

    return exe;
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
        .artifacts_directory = artifacts_directory,
        .current_directory   = cwd,
        .self_build_path     = self_build_path,
    };

    struct Build module = build(&context);
    module.root_dir = ".";

    build_module(&context, &module);

    managed_arena_print((struct Managed_Arena *) scratch.data_context);
    fprintf(stderr, "\n");

    scratch_end(&scratch);
    thread_context_release();
    return 0;
}
