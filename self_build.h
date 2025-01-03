#ifndef SELF_BUILD_H
#define SELF_BUILD_H

char *format_cstring(const char *, ...);

enum Build_Kind {
    Build_Kind_Module,
    Build_Kind_Executable,
};

struct Build {
    char *name;
    enum Build_Kind kind;

    char  **sources;
    size_t  sources_count;

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

#ifdef SELF_BUILD_C
#undef SELF_BUILD_C
#include "self_build.c"
#endif // SELF_BUILD_C
