#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "stdlib/strings.h"
#include "stdlib/win32_platform.h"
#include "stdlib/libc_allocator.h"
#include "stdlib/thread_context.h"
#include "stdlib/scratch_memory.h"

#include <windows.h>
#include <shlwapi.h>

struct String_List win32_list_files_recursive(
    char *directory, char *cwd, char *file_pattern,
    struct Allocator *allocator
) {
    const char *dir     = format_cstring(allocator, "%s/%s", cwd, directory);
    const char *pattern = format_cstring(allocator, "%s/%s", dir, file_pattern);
    fprintf(stderr, "dir: %s\npattern: %s\n", dir, pattern);

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFileA(pattern, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "%s\n", pattern);
        assert(false);
    }

    struct String_List files = { 0 };
    list_init(&files, allocator);

    do {
        bool is_file = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
        char *path = format_cstring(allocator, "%s/%s", directory, ffd.cFileName);

        if (is_file) {
            list_append(&files, cstring_to_string(path, allocator));
            allocator_release(allocator, path);

        } else {
            bool skip = false;
            skip |= strncmp(ffd.cFileName, ".",  strlen(ffd.cFileName)) == 0;
            skip |= strncmp(ffd.cFileName, "..", strlen(ffd.cFileName)) == 0;

            if (!skip) {
                struct String_List subdir_contents = win32_list_files_recursive(path, cwd, file_pattern, allocator);
                for (size_t i = 0; i < list_length(subdir_contents); ++i) {
                    struct String s = subdir_contents.items[i];
                    list_append(&files, s);
                }
            }
        }
    } while (FindNextFile(hFind, &ffd) != 0);


    FindClose(hFind);
    return files;
}

char* replace_all_substrings(const char* original, const char* target, const char* replacement) {
    size_t original_len = strlen(original);
    size_t target_len = strlen(target);
    size_t replacement_len = strlen(replacement);

    // Count the number of occurrences of the target substring
    size_t count = 0;
    const char* pos = original;
    while ((pos = strstr(pos, target)) != NULL) {
        count++;
        pos += target_len;
    }

    // Allocate memory for the new string
    size_t new_str_len = original_len + count * (replacement_len - target_len);
    char* new_str = (char*)malloc(new_str_len + 1);
    if (!new_str) {
        return NULL; // If memory allocation fails, return NULL
    }

    // Replace all occurrences of the target substring
    const char* src = original;
    char* dest = new_str;
    while ((pos = strstr(src, target)) != NULL) {
        // Copy the part of the original string before the target substring
        size_t len = pos - src;
        strncpy(dest, src, len);
        dest += len;

        // Append the replacement substring
        strcpy(dest, replacement);
        dest += replacement_len;

        // Move to the part of the original string after the target substring
        src = pos + target_len;
    }

    // Append the remaining part of the original string
    strcpy(dest, src);
    return new_str; 
}

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

            struct String_List template_files = win32_list_files_recursive("selfbuild/templates/hello_exe", project_dir, "*", &scratch);
            for (size_t i = 0; i < list_length(template_files); ++i) {
                char *source_file = format_cstring(
                    &scratch, "%.*s",
                    (int) template_files.items[i].length,
                    template_files.items[i].data
                );

                char *source_path = format_cstring(
                    &scratch, "%s/%s",
                    project_dir,
                    source_file
                );

                char *destination = replace_all_substrings(source_file, "selfbuild/templates/hello_exe", project_dir);
                fprintf(stderr, "\t%s -> %s\n", source_path, destination);

                char dir[255] = { 0 };
                _splitpath(destination, NULL, dir, NULL, NULL);
                if (!win32_dir_exists(dir)) win32_create_directories(dir);

                win32_copy_file(source_path, destination);
            }

            win32_wait_for_command_format_ex(project_dir, "clang build.c -std=c23 -o build.exe -O0 -Iselfbuild -g -gcodeview -Wl,--pdb=");

        } else {
            fprintf(stderr, "No project name provided\n", cwd);
        }
    }

    scratch_end(&scratch);
    thread_context_release();
    return 0;
}
