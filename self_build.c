#include <stddef.h>
#include <assert.h>

#include <windows.h>

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

int win32_wait_for_command(const char *path, const char *parameters) {
    char command[256] = { 0 };
    sprintf(command, "%s %s", path, parameters);

    STARTUPINFO startup_info = { 0 };
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup_info.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    PROCESS_INFORMATION process_info = { 0 };
    assert(CreateProcessA(
        NULL,
        command,
        NULL, NULL, true, 0, NULL,
        NULL, // @TODO: set working directory
        &startup_info,
        &process_info
    ));
    WaitForSingleObject(process_info.hProcess, INFINITE);

    DWORD exit_code;
    assert(GetExitCodeProcess(process_info.hProcess, &exit_code));

    CloseHandle(process_info.hProcess);
    return exit_code;
}

bool should_recompile(const char *source_file_path, const char *object_file_path) {
    long long source_file_time = win32_get_file_last_modified_time(source_file_path);
    long long object_file_time = win32_get_file_last_modified_time(object_file_path);
    return source_file_time > object_file_time;
}

void build_source_files(struct Build *build, const char *artifacts_directory) {
    for (size_t i = 0; i < build->source_files_count; ++i) {
        char *source_file_path = build->source_files[i];
        char  object_file_path[64] = { 0 };
        sprintf(object_file_path, "%s/%s.o", artifacts_directory, source_file_path);

        if (should_recompile(source_file_path, object_file_path)) {
            char parameters[64] = { 0 };
            sprintf(parameters, "-c %s -o %s", source_file_path, object_file_path);

            fprintf(stderr, "+ clang.exe %s\n", parameters);

            // @TODO: Set working directory to be next to the root build script
            win32_wait_for_command("clang.exe", parameters);
            build->should_recompile = true;
        }
    }
}

void link_objects(struct Build *build, const char *artifacts_directory) {
    if (build->should_recompile) {
        size_t top = 0;
        char all_objects[256] = { 0 };

        for (size_t i = 0; i < build->source_files_count; ++i) {
            // I wish I could do something like this. Maybe I can?
            // printf("%s, %s!", { "Hello", "World" }, 2);

            char object_file_path[64] = { 0 };
            size_t length = sprintf(object_file_path, "%s/%s.o ", artifacts_directory, build->source_files[i]);
            memcpy(&all_objects[top], object_file_path, length);
            top += length;
        }

        char link_parameters[256] = { 0 };
        sprintf(link_parameters, "-o %s/main.exe %s", artifacts_directory, all_objects);

        fprintf(stderr, "+ clang.exe %s\n", link_parameters);
        win32_wait_for_command("clang.exe", link_parameters);
    }
}
