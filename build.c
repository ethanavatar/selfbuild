#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include <windows.h>

#define SELF_BUILD_C
#include "self_build.h"

static const char *artifacts_directory = "bin";

int build(void) {
    // @TODO: The build script should be able work recursively instead of rewriting the root build script
    // to include the source files of a new third party library every time.
    // The way this can be done is with the topmost build script is compiled into an executable,
    // and dependency scripts are compiled into dlls that are dynamically loaded by the root script.
    // They will all use an artifacts folder at the top level to avoid mess in subdirectories

    static char *source_files[] = { "main.c", "other.c" };
    static struct Build build = {
        .kind = Build_Kind_Executable,
        .source_files = source_files,
        .source_files_count = sizeof(source_files) / sizeof(const char *),
        .should_recompile = false,
    };

    if (should_recompile("build.c", "build.exe")) {
        fprintf(stderr, "Bootstrapping...\n");
        MoveFileEx("build.exe", "bin/build.old", MOVEFILE_REPLACE_EXISTING);

        int rebuild_success = win32_wait_for_command("clang.exe", "build.c -o build.exe -std=c23");
        if (rebuild_success == 0) {
            exit(win32_wait_for_command("build.exe", NULL));

        } else {
            MoveFileA("bin/build.old", "build.exe");
        }
    }

    fprintf(stderr, "Building...\n");

    // @TODO: Canonical paths
    if (!win32_path_exists(artifacts_directory)) {
        CreateDirectory(artifacts_directory, NULL);
    }

    build_source_files(&build, artifacts_directory);
    link_objects(&build, artifacts_directory);

    return 0;
}

int main(void) {
    return build();
}
