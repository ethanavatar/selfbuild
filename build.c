#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include <windows.h>

#define SELF_BUILD_C
#include "self_build.h"

struct Build build(void) {

    // @TODO: The build script should be able work recursively instead of rewriting the root build script
    // to include the source files of a new third party library every time.
    // The way this can be done is with the topmost build script is compiled into an executable,
    // and dependency scripts are compiled into dlls that are dynamically loaded by the root script.
    // They will all use an artifacts folder at the top level to avoid mess in subdirectories

    static char *hello_files[] = { "test/hello.c" };
    static struct Build lib = {
        .name = "hello",
        .kind = Build_Kind_Module,
        .source_files = hello_files,
        .source_files_count = sizeof(hello_files) / sizeof(char *),
    };

    static char *main_files[] = { "test/main.c" };
    static struct Build exe = {
        .name = "main",
        .kind = Build_Kind_Executable,
        .source_files = main_files,
        .source_files_count = sizeof(main_files) / sizeof(char *),
    };

    add_dependency(&exe, &lib);
    return exe;
}

static const char *artifacts_directory = "bin";

int main(void) {
    // @TODO: Canonical paths
    // @Refs:
    // - https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamea
    // - https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-pathcanonicalizea
    if (!win32_dir_exists(artifacts_directory)) CreateDirectory(artifacts_directory, NULL);
    bootstrap("build.c", "build.exe", "bin/build.old");

    struct Build module = build();
    build_module(&module, artifacts_directory);
    return 0;
}
