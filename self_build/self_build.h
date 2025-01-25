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

enum Build_Kind {
    Build_Kind_Static_Library,
    Build_Kind_Shared_Library,
    Build_Kind_Executable,
    Build_Kind_COUNT,
};

struct Build {
    char *name;
    enum Build_Kind kind;

    char  **sources;
    size_t  sources_count;

    char  **compile_flags;
    size_t  compile_flags_count;

    char  **link_flags;
    size_t  link_flags_count;

    char  **includes;
    size_t  includes_count;

    bool should_recompile;

    char *root_dir;

    size_t dependencies_count;
    struct Build *dependencies;
};

struct Build_Context {
    char *artifacts_directory;
    char *current_directory;
    char *self_build_path;
};

typedef struct Build (*Build_Function)(struct Build_Context *);

struct Build build_submodule(struct Build_Context *, char *);
size_t build_module(struct Build_Context *, struct Build *);

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

#endif // SELF_BUILD_H
