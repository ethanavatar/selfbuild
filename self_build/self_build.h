#ifndef SELF_BUILD_H_
#define SELF_BUILD_H_

#include "self_build/all_stdlib.h"

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
    bool is_bootstrapped;
};

struct Build {
    struct Build_Context *context;

    char *name;
    enum Build_Kind kind;

    struct String_List sources;
    struct String_List compile_flags;
    struct String_List link_flags;
    struct String_List system_dependencies;
    struct String_List includes;

    bool should_recompile;
    char *root_dir;

    struct Build_List {
        struct List_Header header;
        struct Build *items;
    } dependencies;
};

typedef struct Build (*Build_Function)(struct Build_Context *, enum Build_Kind);

struct Build build_submodule(struct Build_Context *, char *, enum Build_Kind);
size_t build_module(struct Build_Context *, struct Build *);

struct Build build_create(struct Build_Context *context, enum Build_Kind requested_kind, char *name);

bool should_recompile(const char *, const char *);
void bootstrap(
    struct Build_Context *context,
    const char *build_script_path, const char *executable_path
);

void   add_dependency(struct Build *, struct Build);
size_t build_source_files(struct Build_Context *, struct Build *);
void   link_objects(struct Build_Context *, struct Build *);

void build_add_system_library(struct Build *build, char *library_name);
void build_add_include_path(struct Build *build, char *include_path);

struct Build_Context_Options {
    enum Debug_Info_Kind debug_info_kind;
};

struct Build_Context build_create_context(
    struct Build_Context_Options options,
    char *self_build_path,
    char *artifacts_directory,
    struct Allocator *allocator
);

#endif // SELF_BUILD_H_

#ifdef SELF_BUILD_C_
#undef SELF_BUILD_C_
#include "self_build/self_build.c"
#endif // SELF_BUILD_C_
