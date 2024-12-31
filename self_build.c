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
    char *cmd_format = "%s %s";
    int command_length = sprintf(NULL, cmd_format, path, parameters);
    char *command = calloc(command_length + 1, sizeof(char));
    memset(command, 0, command_length + 1);
    sprintf(command, cmd_format, path, parameters);

    STARTUPINFO startup_info = { 0 };
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup_info.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    LPTSTR cwd[MAX_PATH] = { 0 };
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
    free(command);
    return exit_code;
}

bool should_recompile(const char *source_file_path, const char *object_file_path) {
    long long source_file_time = win32_get_file_last_modified_time(source_file_path);
    long long object_file_time = win32_get_file_last_modified_time(object_file_path);
    return source_file_time > object_file_time;
}

void bootstrap(const char *build_script_path, const char *executable_path, const char *old_executable_path) {
    if (should_recompile(build_script_path, executable_path)) {
        fprintf(stderr, "Bootstrapping...\n");
        MoveFileEx(executable_path, old_executable_path, MOVEFILE_REPLACE_EXISTING);

        char arguments[64] = { 0 };
        sprintf(arguments, "%s -o %s -std=c23", build_script_path, executable_path);

        int rebuild_success = win32_wait_for_command("clang.exe", arguments);
        if (rebuild_success == 0) {
            exit(win32_wait_for_command("build.exe", NULL));

        } else {
            MoveFileA("bin/build.old", "build.exe");
        }
    }
}

size_t build_module(struct Build *build, const char *artifacts_directory) {
    fprintf(stderr, "%s dep count: %zu\n", build->name, build->dependencies_count);
    for (size_t dep = 0; dep < build->dependencies_count; ++dep) {
        struct Build *module = build->dependencies[dep];
        size_t compiled = build_module(module, artifacts_directory);
        build->should_recompile = compiled > 0;
        fprintf(stderr, "done module: %s\n", module->name);
    }

    size_t compiled_count = 0;
    fprintf(stderr, "%s source count: %zu\n", build->name, build->source_files_count);

    for (size_t i = 0; i < build->source_files_count; ++i) {
        char *source_file_path = build->source_files[i];
        fprintf(stderr, "building %s\n", source_file_path);

        int path_length = sprintf(NULL, "%s/%s.o", artifacts_directory, source_file_path);
        char *object_file_path = calloc(path_length + 1, sizeof(char));
        object_file_path[path_length] = 0;
        sprintf(object_file_path, "%s/%s.o", artifacts_directory, source_file_path);

        //if (should_recompile(source_file_path, object_file_path)) {
            path_length = sprintf(NULL, "-c %s -o %s", source_file_path, object_file_path);
            char *parameters = calloc(path_length + 1, sizeof(char));
            parameters[path_length] = 0;
            sprintf(parameters, "-c %s -o %s", source_file_path, object_file_path);

            fprintf(stderr, "+ clang.exe %s\n", parameters);

            // @TODO: Set working directory to be next to the root build script
            assert(win32_wait_for_command("clang.exe", parameters) == 0);
            build->should_recompile = true;
        //}
    }

    link_objects(build, artifacts_directory);
    return compiled_count;
}

void link_objects(struct Build *build, const char *artifacts_directory) {
    //if (build->should_recompile) {
        size_t top = 0;
        char all_objects[16] = { 0 };

        for (size_t i = 0; i < build->source_files_count; ++i) {
            // I wish I could do something like this. Maybe I can?
            // printf("%s, %s!", { "Hello", "World" }, 2);

            char object_file_path[64] = { 0 };
            size_t length = sprintf(object_file_path, "%s/%s.o ", artifacts_directory, build->source_files[i]);
            memcpy(&all_objects[top], object_file_path, length);
            top += length;
        }

        if (build->kind == Build_Kind_Module) {
            const char *fmt = "/OUT:%s/%s.lib %s";
            int args_length = sprintf(NULL, fmt, artifacts_directory, build->name, all_objects);
            char *link_parameters = calloc(args_length + 1, sizeof(char));
            link_parameters[args_length] = 0;

            sprintf(link_parameters, fmt, artifacts_directory, build->name, all_objects);

            fprintf(stderr, "+ lib.exe %s\n", link_parameters);
            win32_wait_for_command(
                "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.42.34433/bin/Hostx86/x86/lib.exe",
                link_parameters
            );

        } else if (build->kind == Build_Kind_Executable) {
            const char *fmt = "-o %s/%s.exe %s";
            int args_length = sprintf(NULL, fmt, artifacts_directory, build->name, all_objects);
            char *link_parameters = calloc(args_length + 1, sizeof(char));
            link_parameters[args_length] = 0;

            sprintf(link_parameters, fmt, artifacts_directory, build->name, all_objects);
            fprintf(stderr, "+ clang.exe %s\n", link_parameters);
            win32_wait_for_command("clang.exe", link_parameters);
        }

    //}
}

void add_dependency(struct Build *module, struct Build *dependency) {
    module->dependencies[module->dependencies_count++] = dependency;
    fprintf(stderr, "added dependency \"%s\" to \"%s\"\n", dependency->name, module->name);
}
