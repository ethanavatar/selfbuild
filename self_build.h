#ifndef SELF_BUILD_H
#define SELF_BUILD_H

bool      win32_path_exists(const char *);
long long win32_get_file_last_modified_time(const char *);
void      win32_wait_for_command(const char *, const char *);

enum Build_Kind {
    Build_Kind_Module,
    Build_Kind_Executable,
};

struct Build {
    enum Build_Kind kind;
    char  **source_files;
    size_t  source_files_count;
    bool should_recompile;
};

void build_source_files(struct Build *, const char *);
void link_objects(struct Build *, const char *);

#endif // SELF_BUILD_H

#ifdef SELF_BUILD_C
#undef SELF_BUILD_C
#include "self_build.c"
#endif // SELF_BUILD_C
