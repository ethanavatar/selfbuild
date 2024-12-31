#ifndef SELF_BUILD_H
#define SELF_BUILD_H

bool      win32_dir_exists(const char *);
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

    size_t dependencies_count;
    struct Build *dependencies[8];
};

bool should_recompile(const char *, const char *);
void bootstrap(const char *, const char *, const char *);

void   add_dependency(struct Build *, struct Build *);
size_t build_source_files(struct Build *, const char *);
void   link_objects(struct Build *, const char *);

#endif // SELF_BUILD_H

#ifdef SELF_BUILD_C
#undef SELF_BUILD_C
#include "self_build.c"
#endif // SELF_BUILD_C
