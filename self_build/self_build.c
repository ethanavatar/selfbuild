#include "self_build/self_build.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "stdlib/arena.h"
#include "stdlib/strings.h"
#include "stdlib/allocators.h"
#include "stdlib/win32_platform.h"
#include "stdlib/string_builder.h"
#include "stdlib/scratch_memory.h"

struct Build build_create(struct Build_Context *context, enum Build_Kind kind, char *name) {
    struct Build b = { .kind = kind, .name = name };
    list_init(&b.sources,       &context->allocator);
    list_init(&b.includes,      &context->allocator);
    list_init(&b.compile_flags, &context->allocator);
    list_init(&b.link_flags,    &context->allocator);
    list_init(&b.dependencies,  &context->allocator);
    return b;
}

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

    bool should_exit = false;
    int  exit_code   = 0;

    if (should_recompile(build_script_path, executable_path)) {
        fprintf(stderr, "Bootstrapping...\n");
        win32_move_file(executable_path, old_executable_path, File_Move_Flags_Overwrite);

        int rebuild_success = win32_wait_for_command_format(
            "clang %s -o %s -std=c23 -I%s -O0 -g -gcodeview -Wl,--pdb=",
            build_script_path, executable_path, self_build_path
        );

        if (rebuild_success == 0) {
            should_exit = true;
            exit_code = win32_wait_for_command_ex("build.exe");

        } else {
            win32_move_file(old_executable_path, executable_path, File_Move_Flags_None);
        }
    }

    scratch_end(&scratch);

    if (should_exit) {
        exit(exit_code);
    }
}

struct Build build_submodule(
    struct Build_Context *context, char *module_directory,
    enum Build_Kind requested_kind
) {
    struct Allocator scratch = scratch_begin();

    char *module_artifacts_path = format_cstring(
        &scratch,
        "%s/%s",
        context->artifacts_directory, module_directory
    );

    if (!win32_dir_exists(module_artifacts_path)) {
        win32_create_directories(module_artifacts_path);
    }

    const char *module_dll_path = format_cstring(&scratch, "%s/build.dll", module_artifacts_path);
    win32_wait_for_command_format(
        "clang %s/build.c -I%s -std=c23 -shared -fPIC -o %s -std=c23 -g -gcodeview -Wl,--pdb=",
        module_directory, context->self_build_path, module_dll_path
    );

    void *build_module = win32_load_library(module_dll_path);
    assert(build_module && "failed to load module");

    Build_Function build_function = (Build_Function) win32_get_symbol_address(build_module, "build");

    struct Build_Context submodule_context = { 0 };
    memcpy(&submodule_context, context, sizeof(struct Build_Context));
    submodule_context.current_directory = module_directory;

    struct Build module_definition = build_function(&submodule_context, requested_kind);
    module_definition.root_dir = module_directory;

    scratch_end(&scratch);
    return module_definition;
}

size_t build_module(struct Build_Context *context, struct Build *build) {
    struct Allocator scratch = scratch_begin();
    struct String_Builder sb = string_builder_create(&scratch);

    fprintf(stderr, "Building module: %s\n", build->name);

    for (size_t dep = 0; dep < list_length(build->dependencies); ++dep) {
        struct Build *module = &build->dependencies.items[dep];
        size_t compiled = build_module(context, module);
        build->should_recompile = compiled > 0;

        string_builder_append(&sb, "-I%s ", module->root_dir);

        for (size_t i = 0; i < list_length(module->includes); ++i) {
            struct String flag = build->includes.items[i];
            string_builder_append(&sb, "-I%s/%.*s ", module->root_dir, (int) flag.length, flag.data);
        }
    }

    for (size_t i = 0; i < list_length(build->includes); ++i) {
        struct String flag = build->includes.items[i];
        string_builder_append(&sb, "-I%s/%.*s ", build->root_dir, (int) flag.length, flag.data);
    }

    for (size_t i = 0; i < list_length(build->compile_flags); ++i) {
        struct String flag = build->compile_flags.items[i];
        string_builder_append(&sb, "%.*s ", (int) flag.length, flag.data);
    }

    struct String includes = string_builder_to_string(&sb, &scratch);
    string_builder_clear(&sb);

    size_t compiled_count = 0;
    for (size_t i = 0; i < list_length(build->sources); ++i) {
        struct Allocator sources_scratch = scratch_begin();

        struct String source = build->sources.items[i];

        char *source_file_path = format_cstring(&sources_scratch, "%s/%.*s", build->root_dir, (int) source.length, source.data);
        char *object_file_path = format_cstring(&sources_scratch, "%s/%s.o", context->artifacts_directory, source_file_path);

        char dir[255] = { 0 };
        _splitpath(object_file_path, NULL, dir, NULL, NULL);
        if (!win32_dir_exists(dir)) win32_create_directories(dir);

        // @Bug: This does not handle the case where a file doesnt exist.
        // It just treats it like it doesnt need to be recompiled
        if (should_recompile(source_file_path, object_file_path)) {

            // @TODO: Set working directory to be next to the root build script
            int exit_code = win32_wait_for_command_format(
                "clang -c %s -o %s %.*s -std=c23",
                source_file_path,
                object_file_path,
                (int) includes.length, includes.data
            );
            assert(exit_code == 0);

            build->should_recompile = true;
            compiled_count++;

        } else {

            fprintf(stderr, "skipping %s\n", source_file_path);
        }

        scratch_end(&sources_scratch);
    }

    link_objects(context, build);

    scratch_end(&scratch);
    return compiled_count;
}

// @TODO: find this path programatically
static const char *msvc_lib_path =
    "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.42.34433/bin/Hostx86/x86/lib.exe";

static const char *windows_sdk_path =
    "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x86";

void link_static_library(struct Build_Context *, struct Build *, struct String *);
void link_shared_library(struct Build_Context *, struct Build *, struct String *);
void link_executable    (struct Build_Context *, struct Build *, struct String *);

void link_one(struct Build_Context *context, struct Build *build, struct String *artifacts) {
    static_assert(Build_Kind_COUNT == 3);
    if (0) { }
    else if (build->kind == Build_Kind_Static_Library) { link_static_library(context, build, artifacts); }
    else if (build->kind == Build_Kind_Shared_Library) { link_shared_library(context, build, artifacts); }
    else if (build->kind == Build_Kind_Executable)     { link_executable(context, build, artifacts);     }
}

void link_static_library(struct Build_Context *context, struct Build *build, struct String *artifacts) {
    win32_wait_for_command_format(
        "%s /LIBPATH:\"%s\" /NOLOGO /OUT:%s/%s.lib %.*s",
        msvc_lib_path, windows_sdk_path, context->artifacts_directory, build->name, (int) artifacts->length, artifacts->data
    );
}

void link_shared_library(struct Build_Context *context, struct Build *build, struct String *artifacts) {
    win32_wait_for_command_format(
        "clang -o %s/%s.dll %.*s -std=c23 -shared -fPIC",
        context->artifacts_directory, build->name,
        (int) artifacts->length, artifacts->data
    );
}

void link_executable(struct Build_Context *context, struct Build *build, struct String *artifacts) {
    win32_wait_for_command_format(
        "clang -o %s/%s.exe %.*s -std=c23",
        context->artifacts_directory, build->name,
        (int) artifacts->length, artifacts->data
    );
}

void link_objects(struct Build_Context *context, struct Build *build) {
    if (!build->should_recompile) {
        return;
    }

    struct Allocator scratch = scratch_begin();
    struct String_Builder sb = string_builder_create(&scratch);

    for (size_t i = 0; i < list_length(build->sources); ++i) {
        struct String source = build->sources.items[i];
        string_builder_append(
            &sb, "%s/%s/%.*s.o ",
            context->artifacts_directory, build->root_dir,
            (int) source.length, source.data
        );
    }

    for (size_t i = 0; i < list_length(build->dependencies); ++i) {
        struct Build dependency = build->dependencies.items[i];
        const char *extension = NULL;

        static_assert(Build_Kind_COUNT == 3);
        if (0) { }
        else if (dependency.kind == Build_Kind_Static_Library) { extension = "lib"; }
        else if (dependency.kind == Build_Kind_Shared_Library) { extension = "dll"; }
        else if (dependency.kind == Build_Kind_Executable) {
            assert(false && "cant link against an executable");
        }

        string_builder_append(
            &sb, "%s/%s/%s.%s ",
            context->artifacts_directory, build->root_dir, dependency.name, extension
        );
    }

    for (size_t i = 0; i < list_length(build->link_flags); ++i) {
        struct String flag = build->link_flags.items[i];
        string_builder_append(&sb, "%.*s ", (int) flag.length, flag.data);
    }

    struct String artifacts = string_builder_to_string(&sb, &scratch);
    string_builder_clear(&sb);

    link_one(context, build, &artifacts);

    scratch_end(&scratch);
}

void add_dependency(struct Build *module, struct Build dependency) {
    list_append(&module->dependencies, dependency);
}
