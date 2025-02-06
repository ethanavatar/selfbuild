#include <stdio.h>
#include <assert.h>

#include <windows.h>
#include <shlwapi.h>

#include "stdlib/win32_platform.h"
#include "stdlib/strings.h"

#include "stdlib/allocators.h"
#include "stdlib/arena.h"

#include "stdlib/list.h"

#include "stdlib/scratch_memory.h"
#include "stdlib/string_builder.h"

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

void win32_copy_file(const char *source, const char *destination) {
    CopyFileA((LPCSTR) source, (LPCSTR) destination, FALSE);
}

void *win32_load_library(const char *library) {
    return LoadLibraryA(library);
}

void win32_free_library(void *library) {
    FreeLibrary((HMODULE) library);
}

void *win32_get_symbol_address(void *library, const char *symbol_name) {
    return GetProcAddress((HMODULE) library, symbol_name);
}

// https://stackoverflow.com/a/65949019
static BOOL DirectoryExists(LPCTSTR szPath, BOOL *exists) {
    *exists = FALSE;
    size_t szLen = strlen(szPath);
    if (szLen > 0 && szPath[szLen - 1] == '\\') {
        --szLen;
    }
    
    HANDLE heap = GetProcessHeap();
    char *szPath2 = (char *) HeapAlloc(heap, 0, (szLen + 3) * sizeof(TCHAR));
    
    if (!szPath2) {
        return FALSE;
    }

    CopyMemory(szPath2, szPath, szLen * sizeof(TCHAR));
    szPath2[szLen] = '\\';
    szPath2[szLen + 1] = '.';
    szPath2[szLen + 2] = 0;
    
    WIN32_FILE_ATTRIBUTE_DATA attribs = { 0 };
    BOOL success = GetFileAttributesExA(szPath2, GetFileExInfoStandard, &attribs);
    
    DWORD dwAttrib = attribs.dwFileAttributes;
    
    HeapFree(heap, 0, szPath2);
    
    if (success && dwAttrib != INVALID_FILE_ATTRIBUTES) {
        *exists = TRUE; /* no point checking FILE_ATTRIBUTE_DIRECTORY on "." */
        return TRUE;
    }


    DWORD lastError = GetLastError();
    BOOL  realError = lastError != ERROR_PATH_NOT_FOUND;

    /*
     * If we get anything other than ERROR_PATH_NOT_FOUND then something's wrong.
     * Could be hardware IO, lack of permissions, a symbolic link pointing to somewhere
     * you don't have access, etc.
     */
    return realError;
}

void win32_print_last_error(void) {
    DWORD lastError = GetLastError();
    LPSTR messageBuffer = NULL;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, lastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR) &messageBuffer,
        0, NULL
    );
    fprintf(stderr, "%#8lX: %.*s\n", lastError, (int) size, messageBuffer);
}

// https://stackoverflow.com/a/6218957
bool win32_dir_exists(const char *path) {
#if 0
    DWORD attributes = GetFileAttributesA(path);
    return (attributes != INVALID_FILE_ATTRIBUTES) &&
           (attributes &  FILE_ATTRIBUTE_DIRECTORY);

#else
    BOOL exists = FALSE;
    BOOL ok = DirectoryExists(path, &exists);
    assert(ok);

    return exists == TRUE;
#endif
}

bool win32_file_exists(const char *path) {
    DWORD attributes = GetFileAttributesA(path);
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
    struct String_Builder sb = string_builder_create(&scratch);

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

struct String_List win32_list_files(char *directory, char *file_pattern, struct Allocator *allocator) {
    const char *pattern = format_cstring(allocator, "%s/%s", directory, file_pattern);

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFileA(pattern, &ffd);
    assert(hFind != INVALID_HANDLE_VALUE);

    struct String_List files = { 0 };
    list_init(&files, allocator);

    do {
        bool is_file = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
        if (is_file) {
            char *path = format_cstring(allocator, "%s/%s", directory, ffd.cFileName);
            list_append(&files, cstring_to_string(path, allocator));
            allocator_release(allocator, path);
        }
    } while (FindNextFile(hFind, &ffd) != 0);


    FindClose(hFind);
    return files;
}

void win32_get_executable_dir(char *dir) {
    // @TODO: A fixed buffer allocator like Zig would be great in a place like this
    CHAR lpFilename[MAX_PATH] = { 0 };
    DWORD size = GetModuleFileName(NULL, lpFilename, sizeof(lpFilename));

    _splitpath(lpFilename, NULL, dir, NULL, NULL);
    assert(win32_dir_exists(dir));
}


void *win32_create_file(char *file_path) {
    return CreateFileA(
        file_path,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
}

void win32_close_file(void *handle) {
    CloseHandle(handle);
}

void win32_delete_file(char *file_path) {
    DeleteFile(file_path);
}

