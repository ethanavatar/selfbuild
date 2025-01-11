#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "self_build.h"
#include "win32_platform.h"
#include "strings.h"
#include "allocators.h"
#include "arena.h"
#include "string_builder.h"

#include "scratch_memory.h"

bool should_recompile(const char *source_file_path, const char *object_file_path) {
    long long source_file_time = win32_get_file_last_modified_time(source_file_path);
    long long object_file_time = win32_get_file_last_modified_time(object_file_path);
    return source_file_time > object_file_time;
}

void bootstrap(
    const char *build_script_path,
    const char *executable_path, const char *old_executable_path,
    const char *self_build_path
) {
    struct Allocator scratch = scratch_begin();
    struct String_Builder sb = string_builder_create(&scratch, 0);

    if (should_recompile(build_script_path, executable_path)) {
        fprintf(stderr, "Bootstrapping...\n");
        win32_move_file(executable_path, old_executable_path, File_Move_Flags_Overwrite);

        string_builder_append(&sb, "%s -o %s -std=c23 -I%s", build_script_path, executable_path, self_build_path);
        struct String_View arguments = string_builder_as_string(&sb);
        fprintf(stderr, "+ clang.exe %.*s\n", (int) arguments.length, arguments.data);

        int rebuild_success = win32_wait_for_command("clang.exe", arguments.data);
        if (rebuild_success == 0) {
            exit(win32_wait_for_command("build.exe", NULL));

        } else {
            win32_move_file("bin/build.old", "build.exe", File_Move_Flags_None);
        }
    }

    scratch_end(&scratch);
}

struct Build build_submodule(struct Build_Context *context, char *module_directory) {
    struct Allocator scratch = scratch_begin();

    char *build_script_path = format_cstring(&scratch, "%s/build.c", module_directory);
    assert(win32_file_exists(build_script_path));

    char *module_artifacts_path = format_cstring(&scratch, "%s/%s", context->artifacts_directory, module_directory);
    if (!win32_dir_exists(module_artifacts_path)) {
        win32_create_directories(module_artifacts_path);
    }

    char *module_dll_path = format_cstring(&scratch, "%s/build.dll", module_artifacts_path);
    char *parameters = format_cstring(
        &scratch, 
        "%s -I%s -std=c23 -shared -fPIC -o %s -std=c23",
        build_script_path,
        context->self_build_path,
        module_dll_path
    );
    win32_wait_for_command("clang.exe", parameters);

    void *build_module = win32_load_library(module_dll_path);
    assert(build_module && "failed to load module");

    Build_Function build_function = (Build_Function) win32_get_symbol_address(build_module, "build");

    struct Build_Context submodule_context = { 0 };
    memcpy(&submodule_context, context, sizeof(struct Build_Context));

    submodule_context.current_directory = module_directory;

    struct Build module_definition = build_function(&submodule_context);
    module_definition.root_dir = module_directory;

    scratch_end(&scratch);
    return module_definition;
}

size_t build_module(struct Build_Context *context, struct Build *build) {
    struct Allocator scratch = scratch_begin();

    // @TODO: resizable arrays
    size_t includes_length = 0;
    char includes[256] = { 0 };

    for (size_t dep = 0; dep < build->dependencies_count; ++dep) {
        struct Build *module = &build->dependencies[dep];
        size_t compiled = build_module(context, module);
        build->should_recompile = compiled > 0;

        char *include = format_cstring(&scratch, "-I%s ", module->root_dir);
        memcpy(&includes[includes_length], include, strlen(include));
        includes_length += strlen(include);

        for (size_t i = 0; i < module->includes_count; ++i) {
            include = format_cstring(&scratch, "-I%s ", module->includes[i]);
            memcpy(&includes[includes_length], include, strlen(include));
            includes_length += strlen(include);
        }
    }

    for (size_t i = 0; i < build->includes_count; ++i) {
        char *include = format_cstring(&scratch, "-I%s ", build->includes[i]);
        memcpy(&includes[includes_length], include, strlen(include));
        includes_length += strlen(include);
    }

    size_t compiled_count = 0;
    for (size_t i = 0; i < build->sources_count; ++i) {
        char *source_file_path = format_cstring(&scratch, "%s/%s", build->root_dir, build->sources[i]);
        char *object_file_path = format_cstring(&scratch, "%s/%s.o", context->artifacts_directory, source_file_path);

        char dir[255] = { 0 };
        _splitpath(object_file_path, NULL, dir, NULL, NULL);
        if (!win32_dir_exists(dir)) win32_create_directories(dir);

        // @Bug: This does not handle the case where a file doesnt exist.
        // It just treats it like it doesnt need to be recompiled
        if (should_recompile(source_file_path, object_file_path)) {
            char *parameters = format_cstring(&scratch, "-c %s -o %s %s -std=c23", source_file_path, object_file_path, includes);
            fprintf(stderr, "+ clang.exe %s\n", parameters);

            // @TODO: Set working directory to be next to the root build script
            assert(win32_wait_for_command("clang.exe", parameters) == 0);
            build->should_recompile = true;
            compiled_count++;

        } else {

            fprintf(stderr, "skipping %s\n", source_file_path);
        }
    }

    link_objects(context, build);

    scratch_end(&scratch);
    return compiled_count;
}

void link_objects(struct Build_Context *context, struct Build *build) {

    if (build->should_recompile) {

        struct Allocator scratch = scratch_begin();

        size_t top = 0;
        char all_objects[255] = { 0 };

        for (size_t i = 0; i < build->sources_count; ++i) {
            // I wish I could do something like this. Maybe I can?
            // printf("%s, %s!", { "Hello", "World" }, 2);

            char *object_file_path = format_cstring(
                &scratch, 
                "%s/%s/%s.o ",
                context->artifacts_directory,
                build->root_dir,
                build->sources[i]
            );

            memcpy(&all_objects[top], object_file_path, strlen(object_file_path));
            top += strlen(object_file_path);
        }

        for (size_t i = 0; i < build->dependencies_count; ++i) {

            char *library_file_path = format_cstring(
                &scratch, 
                "%s/%s/%s.lib ",
                context->artifacts_directory,
                build->root_dir,
                build->dependencies[i].name
            );

            memcpy(&all_objects[top], library_file_path, strlen(library_file_path));
            top += strlen(library_file_path);
        }

        if (build->kind == Build_Kind_Module) {
            //fprintf(stderr, "%s is a Module \n", build->name);
            char *link_parameters = format_cstring(&scratch, "/NOLOGO /OUT:%s/%s.lib %s", context->artifacts_directory, build->name, all_objects);

            fprintf(stderr, "+ lib.exe %s\n", link_parameters);
            win32_wait_for_command(
                // @TODO: find this path programatically
                "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.42.34433/bin/Hostx86/x86/lib.exe",
                link_parameters
            );

        } else if (build->kind == Build_Kind_Executable) {
            //fprintf(stderr, "%s is an Executable \n", build->name);
            char *link_parameters = format_cstring(&scratch, "-o %s/%s.exe %s -std=c23", context->artifacts_directory, build->name, all_objects);

            fprintf(stderr, "+ clang.exe %s\n", link_parameters);
            win32_wait_for_command("clang.exe", link_parameters);
        }

        scratch_end(&scratch);
    }
}

void add_dependency(struct Build *module, struct Build dependency) {
    module->dependencies[module->dependencies_count++] = dependency;
}
