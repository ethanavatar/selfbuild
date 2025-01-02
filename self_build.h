#ifndef SELF_BUILD_H
#define SELF_BUILD_H

char *format_cstring(const char *, ...);

bool      win32_dir_exists(const char *);
bool      win32_file_exists(const char *);
long long win32_get_file_last_modified_time(const char *);
int       win32_wait_for_command(const char *, const char *);

enum Build_Kind {
    Build_Kind_Module,
    Build_Kind_Executable,
};

struct Build {
    char *name;
    enum Build_Kind kind;

    char  **source_files;
    size_t  source_files_count;

    bool should_recompile;

    char *root_dir;

    size_t dependencies_count;
    struct Build *dependencies;
};

struct Build_Context {
    char *artifacts_directory;
    char *current_directory;
};

typedef struct Build (*Build_Function)(struct Build_Context *);

bool should_recompile(const char *, const char *);
void bootstrap(const char *, const char *, const char *);

void   add_dependency(struct Build *, struct Build);
size_t build_source_files(struct Build_Context *, struct Build *);
void   link_objects(struct Build_Context *, struct Build *);

#endif // SELF_BUILD_H

#ifdef SELF_BUILD_C
#undef SELF_BUILD_C
#include "self_build.c"
#endif // SELF_BUILD_C
