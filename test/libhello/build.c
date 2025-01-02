#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include <windows.h>

#define SELF_BUILD_C
#include "self_build.h"

extern struct Build __declspec(dllexport) build(struct Build_Context *);

struct Build build(struct Build_Context *context) {

    static char *hello_files[] = { "hello.c" };
    static struct Build lib = {
        .name = "hello",
        .kind = Build_Kind_Module,
        .source_files = hello_files,
        .source_files_count = sizeof(hello_files) / sizeof(char *),
    };

    return lib;
}


int main(void) {
    char *artifacts_directory = "bin";

    // @TODO: Canonical paths
    // @Refs:
    // - https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamea
    // - https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-pathcanonicalizea
    if (!win32_dir_exists(artifacts_directory)) CreateDirectory(artifacts_directory, NULL);
    bootstrap("build.c", "build.exe", "bin/build.old");

    struct Build_Context context = {
        .artifacts_directory = artifacts_directory,
        //.current_directory = "" // @TODO
    };

    struct Build module = build(&context);
    build_module(&context, &module);
    return 0;
}
