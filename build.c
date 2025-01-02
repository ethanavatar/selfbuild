#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include <windows.h>

#define SELF_BUILD_C
#include "self_build.h"

struct Build build_submodule(struct Build_Context *context, char *module_directory) {
    char *build_script_path = format_cstring("%s/build.c", module_directory);
    assert(win32_file_exists(build_script_path));

    char *module_artifacts_path = format_cstring("%s/%s", context->artifacts_directory, module_directory);
    if (!win32_dir_exists(module_artifacts_path)) {
        fprintf(stderr, "Creating Directory: %s\n", module_artifacts_path);

        // @TODO: Recursive mkdir
        assert(CreateDirectory("bin/test", NULL));
        assert(CreateDirectory("bin/test/libhello", NULL));
    }

    char *module_dll_path = format_cstring("%s/build.dll", module_artifacts_path);
    char *parameters = format_cstring("%s -I. -std=c23 -shared -fPIC -o %s", build_script_path, module_dll_path);
    win32_wait_for_command("clang.exe", parameters);
    free(parameters);

    HMODULE build_module = LoadLibraryA(module_dll_path);
    assert(build_module && "failed to load module");
    free(module_dll_path);
    free(module_artifacts_path);

    Build_Function build_function = (Build_Function) GetProcAddress(build_module, "build");

    struct Build_Context submodule_context = { 0 };
    memcpy(&submodule_context, context, sizeof(struct Build_Context));

    submodule_context.current_directory = module_directory;

    struct Build module_definition = build_function(&submodule_context);
    module_definition.root_dir = module_directory;

    //FreeLibrary(build_module);

    return module_definition;
}

extern struct Build __declspec(dllexport) build(struct Build_Context *);

struct Build build(struct Build_Context *context) {

    struct Build libhello = build_submodule(context, "test/libhello");
    libhello.name = "hello";

    static char *main_files[] = { "test/main.c" };
    static struct Build exe = {
        .name = "main",
        .root_dir = ".",
        .kind = Build_Kind_Executable,
        .source_files = main_files,
        .source_files_count = sizeof(main_files) / sizeof(char *),
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
    if (!win32_dir_exists(artifacts_directory)) CreateDirectory(artifacts_directory, NULL);
    bootstrap("build.c", "build.exe", "bin/build.old");

    char cwd[MAX_PATH] = { 0 };
    DWORD a = GetCurrentDirectory(MAX_PATH, cwd);

    struct Build_Context context = {
        .artifacts_directory = artifacts_directory,
        .current_directory   = cwd,
    };

    struct Build module = build(&context);
    build_module(&context, &module);
    return 0;
}
