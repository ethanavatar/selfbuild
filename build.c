#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include <windows.h>

#define SELF_BUILD_C
#include "self_build.h"

static const char *artifacts_directory = "bin";

void build(void) {
    // @TODO: The build script should be able work recursively instead of rewriting the root build script
    // to include the source files of a new third party library every time.
    // The way this can be done is with the topmost build script is compiled into an executable,
    // and dependency scripts are compiled into dlls that are dynamically loaded by the root script.
    // They will all use an artifacts folder at the top level to avoid mess in subdirectories

    static const char *source_files[] = { "main.c", "other.c" };
    static struct Build build = {
        .kind = Build_Kind_Executable,
        .source_files = source_files,
        .source_files_count = sizeof(source_files) / sizeof(const char *),
        .should_recompile = false,
    };

    // @TODO: Canonical paths
    if (!win32_path_exists(artifacts_directory)) {
        CreateDirectory(artifacts_directory, NULL);
    }

    build_source_files(&build, artifacts_directory);
    link_objects(&build, artifacts_directory);
}

int main(void) {
    build();
    return 0;
}
