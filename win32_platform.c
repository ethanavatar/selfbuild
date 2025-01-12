#include <stdio.h>
#include <assert.h>

#include <windows.h>
#include <shlwapi.h>

#include "win32_platform.h"
#include "strings.h"

#include "allocators.h"
#include "arena.h"

#include "scratch_memory.h"
#include "string_builder.h"

char *win32_get_current_directory(struct Allocator *allocator) {
    char cwd[MAX_PATH] = { 0 };
    DWORD bytes_written = GetCurrentDirectory(MAX_PATH, cwd);
    
    char *result = allocator_allocate(allocator, sizeof(char) * (bytes_written + 1));
    memcpy(result, cwd, bytes_written + 1);

    return result;
}

void win32_move_file(const char *source, const char *destination, enum File_Move_Flags flags) {
    MoveFileEx(source, destination, flags);
}

void *win32_load_library(const char *library) {
    return LoadLibraryA(library);
}

void *win32_get_symbol_address(void *library, const char *symbol_name) {
    return GetProcAddress((HMODULE) library, symbol_name);
}

// https://stackoverflow.com/a/6218957
bool win32_dir_exists(const char *path) {
    DWORD attributes = GetFileAttributes(path);
    return (attributes != INVALID_FILE_ATTRIBUTES) &&
           (attributes &  FILE_ATTRIBUTE_DIRECTORY);
}

bool win32_file_exists(const char *path) {
    DWORD attributes = GetFileAttributes(path);
    return (attributes != INVALID_FILE_ATTRIBUTES) &&
           (attributes &  FILE_ATTRIBUTE_DIRECTORY) == 0;
}

long long win32_get_file_last_modified_time(const char *path) {
    long long last_modified_time = 0;

    HANDLE source_fh = CreateFile(
        path,
        GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL
    );

    if (source_fh != INVALID_HANDLE_VALUE) {
        FILETIME ft = { 0 };
        assert(GetFileTime(source_fh, NULL, NULL, &ft));
        CloseHandle(source_fh);

        ULARGE_INTEGER lv_Large;
        lv_Large.LowPart  = ft.dwLowDateTime;
        lv_Large.HighPart = ft.dwHighDateTime;
        last_modified_time = lv_Large.QuadPart;
    }

    return last_modified_time;
}

int win32_wait_for_command(
    const char *path, const char *parameters
) {
    struct Allocator scratch = scratch_begin();
    char *command = format_cstring(&scratch, "%s %s", path, parameters);
    int exit_code = win32_wait_for_command_ex(command);
    scratch_end(&scratch);

    return exit_code;
}

int win32_wait_for_command_ex(char *command) {
    STARTUPINFO startup_info = { 0 };
    startup_info.cb         = sizeof(startup_info);
    startup_info.dwFlags    = STARTF_USESTDHANDLES;
    startup_info.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    startup_info.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup_info.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

    char cwd[MAX_PATH] = { 0 };
    DWORD a = GetCurrentDirectory(MAX_PATH, cwd);

    PROCESS_INFORMATION process_info = { 0 };
    CreateProcessA(
        NULL,
        command,
        NULL, NULL, true, 0, NULL,
        cwd,
        &startup_info,
        &process_info
    );
    WaitForSingleObject(process_info.hProcess, INFINITE);

    DWORD exit_code;
    GetExitCodeProcess(process_info.hProcess, &exit_code);

    CloseHandle(process_info.hProcess);
    return exit_code;
}

int win32_wait_for_command_format(const char *format, ...) {
    struct Allocator scratch = scratch_begin();
    struct String_Builder sb = string_builder_create(&scratch, 0);

    va_list format_args;
    va_start(format_args, format);
    string_builder_append_vargs(&sb, format, format_args);
    va_end(format_args);

    struct String_View command = string_builder_as_string(&sb);
    fprintf(stderr, "+ %.*s\n", (int) command.length, command.data);
    int exit_code = win32_wait_for_command_ex(command.data);

    scratch_end(&scratch);
    return exit_code;
}


void win32_create_directories(const char *path) {
    char temp[MAX_PATH];
    char *p = NULL;
    size_t len;
    snprintf(temp, sizeof(temp), "%s", path);
    len = strlen(temp);
    if (temp[len - 1] == '/') {
        temp[len - 1] = '\0';
    }

    for (p = temp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            CreateDirectory(temp, NULL);
            *p = '/';
        }
    }

    CreateDirectory(temp, NULL);
}

