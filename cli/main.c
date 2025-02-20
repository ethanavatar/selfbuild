#include <stdio.h>
#include <string.h>
#include "stdlib/strings.h"
#include "stdlib/win32_platform.h"
#include "stdlib/libc_allocator.h"
#include "stdlib/thread_context.h"
#include "stdlib/scratch_memory.h"

int main(int argc, char *argv[]) {
    struct Thread_Context tctx = { 0 };
    thread_context_init_and_equip(&tctx);

    struct Allocator scratch = scratch_begin(NULL);

    char *cwd = win32_get_current_directory(&scratch);
    fprintf(stderr, "cwd: %s\n", cwd);

    for (size_t i = 0; i < argc; ++i) {
        fprintf(stderr, "%zu: \"%s\"\n", i, argv[i]);
    }

    if (argc >= 2 && strcmp(argv[1], "new") == 0) {

        // @TODO: Create unique names incase there isn't one passed
        char *project_name = NULL;
        if (argc >= 3) {
            project_name = argv[2];
        }

        if (project_name != NULL) {
            char *project_dir = format_cstring(&scratch, "%s/%s", cwd, project_name);
            win32_create_directories(project_dir);
            win32_wait_for_command_format_ex(project_dir, "git init .");
            win32_wait_for_command_format_ex(project_dir, "git submodule add git@github.com:ethanavatar/selfbuild.git");

            struct String_List template_files = win32_list_files("selfbuild/templates/hello_exe", project_dir, "**/*", &scratch);
            for (size_t i = 0; i < list_length(template_files); ++i) {
                struct String file = template_files.items[i];
                fprintf(stderr, "\t%.*s", (int) file.length, file.data);
            }


        } else {
            fprintf(stderr, "No project name provided\n", cwd);
        }
    }

    scratch_end(&scratch);
    thread_context_release();
    return 0;
}
