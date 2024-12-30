#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include <windows.h>

static const char *artifacts_directory = "bin";

// https://stackoverflow.com/a/6218957
bool win32_path_exists(const char *path) {
    DWORD attributes = GetFileAttributes(path);
    return attributes != INVALID_FILE_ATTRIBUTES;
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

void build(void) {
    // @TODO: The build script should be able work recursively instead of rewriting the root build script
    // to include the source files of a new third party library every time.
    // The way this can be done is with the topmost build script is compiled into an executable,
    // and dependency scripts are compiled into dlls that are dynamically loaded by the root script.
    // They will all use an artifacts folder at the top level to avoid mess in subdirectories


    static const char  *source_files[]     = { "main.c" };
    static const size_t source_files_count = sizeof(source_files) / sizeof(const char *);


    // @TODO: Canonical paths
    if (!win32_path_exists(artifacts_directory)) {
        CreateDirectory(artifacts_directory, NULL);
    }

    // @TODO: Probably want to move this into CreateProcessA for pipe control
    // @Ref: https://stackoverflow.com/a/31572351
    for (size_t i = 0; i < source_files_count; ++i) {

        char *source_file_path = source_files[i];
        char  object_file_path[64] = { 0 };
        sprintf(object_file_path, "%s/%s.o", artifacts_directory, source_file_path);

        long long source_ft = win32_get_file_last_modified_time(source_file_path);
        long long object_ft = win32_get_file_last_modified_time(object_file_path);

        if (source_ft > object_ft) {
            char parameters[256] = { 0 };
            sprintf(parameters, "%s -o %s", source_file_path, object_file_path);

            fprintf(stderr, "+ clang.exe %s\n", parameters);

            // @TODO: Set working directory to be next to the root build script
            ShellExecuteA(NULL, "open", "clang.exe", parameters, NULL, 0);
        }
    }
}

int main(void) {
    build();
    return 0;
}
