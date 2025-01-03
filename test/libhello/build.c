#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include <windows.h>

#define SELF_BUILD_C
#include "self_build.h"

extern struct Build __declspec(dllexport) build(struct Build_Context *);

struct Build build(struct Build_Context *context) {

    static char *hello_files[] = { "src/hello.c" };
    static char *includes[]    = { "src" };
    static struct Build lib = {
        .kind = Build_Kind_Module,
        .name = "hello",

        .sources        = hello_files,
        .sources_count  = sizeof(hello_files) / sizeof(char *),

        .includes       = includes,
        .includes_count = sizeof(includes) / sizeof(char *),
    };

    return lib;
}


int main(void) {
    char *artifacts_directory = "bin";

    // @TODO: Canonical paths
    if (!win32_dir_exists(artifacts_directory)) win32_create_directories(artifacts_directory);
    bootstrap("build.c", "build.exe", "bin/build.old", "../..");

    struct Build_Context context = {
        .artifacts_directory = artifacts_directory,
        .self_build_path = "../.."
        //.current_directory = "" // @TODO
    };

    struct Build module = build(&context);
    build_module(&context, &module);
    return 0;
}
