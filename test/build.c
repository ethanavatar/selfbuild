#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include <windows.h>

#define SELF_BUILD_C
#include "self_build.h"

extern struct Build __declspec(dllexport) build(struct Build_Context *);

struct Build build(struct Build_Context *context) {

    struct Build libhello = build_submodule(context, "libhello");

    static char *main_files[] = { "main.c" };
    static char *includes[]   = { "libhello/src" };
    static struct Build exe = {
        .kind = Build_Kind_Executable,
        .name = "main",

        .sources       = main_files,
        .sources_count = sizeof(main_files) / sizeof(char *),

        .includes       = includes,
        .includes_count = sizeof(includes) / sizeof(char *)
    };

    exe.dependencies = calloc(8, sizeof(struct Build));
    add_dependency(&exe, libhello);

    return exe;
}

int main(void) {
    char *artifacts_directory = "bin";

    // @TODO: Canonical paths
    // @Refs:
    // - https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamea
    // - https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-pathcanonicalizea
    if (!win32_dir_exists(artifacts_directory)) win32_create_directories(artifacts_directory);
    bootstrap("build.c", "build.exe", "bin/build.old", "..");

    char cwd[MAX_PATH] = { 0 };
    DWORD a = GetCurrentDirectory(MAX_PATH, cwd);

    struct Build_Context context = {
        .artifacts_directory = artifacts_directory,
        .current_directory   = cwd,
        .self_build_path     = ".."
    };

    struct Build module = build(&context);
    module.root_dir = ".";

    build_module(&context, &module);

    return 0;
}
