#include <stddef.h>
#include <stdarg.h>
#include <assert.h>

#include <windows.h>
#include <shlwapi.h>

// @Note: Interesting read
// https://stackoverflow.com/questions/52537188/format-strings-safely-when-vsnprintf-is-not-available
char *format_cstring(const char *format, ...) {
    char *result = NULL;
    va_list args;
    va_start(args, format);

    int size = vsnprintf(NULL, 0, format, args);
    if (size > 0) {
        result = calloc(size + 1, sizeof(char));
        memset(result, 0, size + 1);
        vsnprintf(result, size + 1, format, args);
    }

    va_end(args);
    return result;
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

int win32_wait_for_command(const char *path, const char *parameters) {
    char *command = format_cstring("%s %s", path, parameters);

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
    free(command);
    return exit_code;
}

bool should_recompile(const char *source_file_path, const char *object_file_path) {
    long long source_file_time = win32_get_file_last_modified_time(source_file_path);
    long long object_file_time = win32_get_file_last_modified_time(object_file_path);
    return source_file_time > object_file_time;
}

void bootstrap(
    const char *build_script_path,
    const char *executable_path,
    const char *old_executable_path,
    const char *self_build_path
) {
    if (should_recompile(build_script_path, executable_path)) {
        fprintf(stderr, "Bootstrapping...\n");
        MoveFileEx(executable_path, old_executable_path, MOVEFILE_REPLACE_EXISTING);

        char arguments[64] = { 0 };
        sprintf(arguments, "%s -o %s -std=c23 -I%s", build_script_path, executable_path, self_build_path);
        fprintf(stderr, "+ clang.exe %s", arguments);

        int rebuild_success = win32_wait_for_command("clang.exe", arguments);
        if (rebuild_success == 0) {
            exit(win32_wait_for_command("build.exe", NULL));

        } else {
            MoveFileA("bin/build.old", "build.exe");
        }
    }
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

struct Build build_submodule(struct Build_Context *context, char *module_directory) {
    char *build_script_path = format_cstring("%s/build.c", module_directory);
    //fprintf(stderr, "script path: %s", build_script_path);
    assert(win32_file_exists(build_script_path));

    char *module_artifacts_path = format_cstring("%s/%s", context->artifacts_directory, module_directory);
    if (!win32_dir_exists(module_artifacts_path)) {
        win32_create_directories(module_artifacts_path);
    }

    char *module_dll_path = format_cstring("%s/build.dll", module_artifacts_path);
    char *parameters = format_cstring(
        "%s -I%s -std=c23 -shared -fPIC -o %s",
        build_script_path,
        context->self_build_path,
        module_dll_path
    );
    win32_wait_for_command("clang.exe", parameters);
    free(parameters);

    HMODULE build_module = LoadLibraryA(module_dll_path);
    assert(build_module && "failed to load module");
    free(module_dll_path);
    free(module_artifacts_path);

    Build_Function build_function = (Build_Function) GetProcAddress(build_module, "build");

    struct Build_Context submodule_context = { 0 };
    memcpy(&submodule_context, context, sizeof(struct Build_Context));

    submodule_context.current_directory = module_directory;

    struct Build module_definition = build_function(&submodule_context);
    module_definition.root_dir = module_directory;

    return module_definition;
}

size_t build_module(struct Build_Context *context, struct Build *build) {

    size_t includes_length = 0;
    char includes[256] = { 0 };

    for (size_t dep = 0; dep < build->dependencies_count; ++dep) {
        struct Build *module = &build->dependencies[dep];
        size_t compiled = build_module(context, module);
        build->should_recompile = compiled > 0;

        char *include = format_cstring("-I%s ", module->root_dir);
        memcpy(&includes[includes_length], include, strlen(include));
        includes_length += strlen(include);
        free(include);

        for (size_t i = 0; i < module->includes_count; ++i) {
            include = format_cstring("-I%s ", module->includes[i]);
            memcpy(&includes[includes_length], include, strlen(include));
            includes_length += strlen(include);
            free(include);
        }
    }

    for (size_t i = 0; i < build->includes_count; ++i) {
        char *include = format_cstring("-I%s ", build->includes[i]);
        memcpy(&includes[includes_length], include, strlen(include));
        includes_length += strlen(include);
        free(include);
    }

    size_t compiled_count = 0;
    for (size_t i = 0; i < build->sources_count; ++i) {
        char *source_file_path = format_cstring("%s/%s", build->root_dir, build->sources[i]);
        char *object_file_path = format_cstring("%s/%s.o", context->artifacts_directory, source_file_path);

        char dir[MAX_PATH];
        _splitpath(object_file_path, NULL, dir, NULL, NULL);
        if (!win32_dir_exists(dir)) win32_create_directories(dir);

        // @Bug: This does not handle the case where a file doesnt exist.
        // It just treats it like it doesnt need to be recompiled
        if (should_recompile(source_file_path, object_file_path)) {
            char *parameters = format_cstring("-c %s -o %s %s", source_file_path, object_file_path, includes);
            fprintf(stderr, "+ clang.exe %s\n", parameters);

            // @TODO: Set working directory to be next to the root build script
            assert(win32_wait_for_command("clang.exe", parameters) == 0);
            build->should_recompile = true;
            compiled_count++;

            free(parameters);
        } else {

            fprintf(stderr, "skipping %s\n", source_file_path);
        }

        free(object_file_path);
    }

    link_objects(context, build);
    return compiled_count;
}

void link_objects(struct Build_Context *context, struct Build *build) {
    if (build->should_recompile) {
        size_t top = 0;
        char all_objects[255] = { 0 };

        for (size_t i = 0; i < build->sources_count; ++i) {
            // I wish I could do something like this. Maybe I can?
            // printf("%s, %s!", { "Hello", "World" }, 2);

            char *object_file_path = format_cstring(
                "%s/%s/%s.o ",
                context->artifacts_directory,
                build->root_dir,
                build->sources[i]
            );

            memcpy(&all_objects[top], object_file_path, strlen(object_file_path));
            top += strlen(object_file_path);
            free(object_file_path);
        }

        for (size_t i = 0; i < build->dependencies_count; ++i) {

            char *library_file_path = format_cstring(
                "%s/%s/%s.lib ",
                context->artifacts_directory,
                build->root_dir,
                build->dependencies[i].name
            );

            memcpy(&all_objects[top], library_file_path, strlen(library_file_path));
            top += strlen(library_file_path);
            free(library_file_path);
        }

        if (build->kind == Build_Kind_Module) {
            //fprintf(stderr, "%s is a Module \n", build->name);
            char *link_parameters = format_cstring("/NOLOGO /OUT:%s/%s.lib %s", context->artifacts_directory, build->name, all_objects);

            fprintf(stderr, "+ lib.exe %s\n", link_parameters);
            win32_wait_for_command(
                // @TODO: find this path programatically
                "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.42.34433/bin/Hostx86/x86/lib.exe",
                link_parameters
            );
            free(link_parameters);

        } else if (build->kind == Build_Kind_Executable) {
            //fprintf(stderr, "%s is an Executable \n", build->name);
            char *link_parameters = format_cstring("-o %s/%s.exe %s", context->artifacts_directory, build->name, all_objects);

            fprintf(stderr, "+ clang.exe %s\n", link_parameters);
            win32_wait_for_command("clang.exe", link_parameters);

            free(link_parameters);
        }

    }
}

void add_dependency(struct Build *module, struct Build dependency) {
    module->dependencies[module->dependencies_count++] = dependency;
}
