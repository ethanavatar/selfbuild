#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#define SELF_BUILD_C
#include "self_build.h"

#include "win32_platform.h"
#include "win32_platform.c"

#include "strings.h"
#include "strings.c"

#include "allocators.h"
#include "allocators.c"

#include "arena.h"
#include "arena.c"

#include "thread_context.h"
#include "thread_context.c"

#include "managed_arena.h"
#include "managed_arena.c"

#include "scratch_memory.h"
#include "scratch_memory.c"

#include "string_builder.h"
#include "string_builder.c"

extern struct Build __declspec(dllexport) build(struct Build_Context *);

struct Build build(struct Build_Context *context) {

    static char *main_files[] = {
        "self_build.c",
        "win32_platform.c",
        "strings.c",

        "allocators.c",
        "arena.c", "managed_arena.c",
        "thread_context.c",
        "scratch_memory.c",
        "string_builder.c",
    };

    static struct Build exe = {
        .kind = Build_Kind_Module,
        .name = "self_build",

        .sources        = main_files,
        .sources_count  = sizeof(main_files) / sizeof(char *),

        .includes       = NULL,
        .includes_count = 0,
    };

    return exe;
}

int main(void) {
    struct Thread_Context tctx;
    thread_context_init_and_equip(&tctx);

    struct Allocator scratch = scratch_begin();

    char *artifacts_directory = "bin";
    char *self_build_path     = ".";

    // @TODO: Canonical paths
    // @Refs:
    // - https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamea
    // - https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-pathcanonicalizea
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
