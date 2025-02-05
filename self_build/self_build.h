#ifndef SELF_BUILD_H
#define SELF_BUILD_H

#include "stdlib/win32_platform.h"
#include "stdlib/strings.h"
#include "stdlib/allocators.h"
#include "stdlib/arena.h"
#include "stdlib/thread_context.h"
#include "stdlib/managed_arena.h"
#include "stdlib/scratch_memory.h"
#include "stdlib/string_builder.h"
#include "stdlib/list.h"

enum Build_Kind {
    Build_Kind_Static_Library,
    Build_Kind_Shared_Library,
    Build_Kind_Executable,
    Build_Kind_COUNT,
};

enum Debug_Info_Kind {
    Debug_Info_Kind_None,
    Debug_Info_Kind_Portable,
    Debug_Info_Kind_Embedded,
};

struct Build_Context {
    char *artifacts_directory;
    char *current_directory;
    char *self_build_path;
    struct Allocator allocator;

    enum Debug_Info_Kind debug_info_kind;
};

struct Build_Options {
    enum Build_Kind build_kind;
};

struct Build {
    struct Build_Context *context;

    char *name;
    struct Build_Options options;

    struct String_List sources;
    struct String_List compile_flags;
    struct String_List system_dependencies;
    struct String_List includes;

    bool should_recompile;
    char *root_dir;

    struct Build_List {
        struct List_Header header;
        struct Build *items;
    } dependencies;
};

typedef struct Build (*Build_Function)(struct Build_Context *, struct Build_Options);

struct Build build_submodule(struct Build_Context *, char *, struct Build_Options);
size_t build_module(struct Build_Context *, struct Build *);

struct Build build_create(struct Build_Context *context, struct Build_Options options, char *name);

bool should_recompile(const char *, const char *);
void bootstrap(
    const char *build_script_path,
    const char *executable_path,
    const char *old_executable_path,
    const char *self_build_path
);

void   add_dependency(struct Build *, struct Build);
size_t build_source_files(struct Build_Context *, struct Build *);
void   link_objects(struct Build_Context *, struct Build *);

void build_add_system_library(struct Build *build, char *library_name);

#endif // SELF_BUILD_H
