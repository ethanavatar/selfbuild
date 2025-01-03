#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#define SELF_BUILD_C
#include "self_build.h"

#include "win32_platform.h"
#include "win32_platform.c"

#include "strings.h"
#include "strings.c"

extern struct Build __declspec(dllexport) build(struct Build_Context *);

struct Build build(struct Build_Context *context) {

    static char *main_files[] = {
        "self_build.c",
        "win32_platform.c",
        "strings.c"
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
    char *artifacts_directory = "bin";
    char *self_build_path     = ".";

    // @TODO: Canonical paths
    // @Refs:
    // - https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamea
    // - https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-pathcanonicalizea
    if (!win32_dir_exists(artifacts_directory)) win32_create_directories(artifacts_directory);
    bootstrap("build.c", "build.exe", "bin/build.old", "..");

    char *cwd = win32_get_current_directory();

    struct Build_Context context = {
        .artifacts_directory = artifacts_directory,
        .current_directory   = cwd,
        .self_build_path     = self_build_path,
    };

    struct Build module = build(&context);
    module.root_dir = ".";

    build_module(&context, &module);

    free(cwd);
    return 0;
}
