#if defined(_WIN32)
#include "stdlib/file_io.h"

#include <windows.h>

struct File_Contents file_read_to_end(
    const char *file_path,
    struct Allocator *allocator
) {
    struct File_Contents result = { 0 };
    HANDLE file_handle = CreateFileA(
        file_path,
        GENERIC_READ, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    bool should_read = file_handle != INVALID_HANDLE_VALUE;

    if (should_read) {
        LARGE_INTEGER size = { 0 };
        should_read = GetFileSizeEx(file_handle, &size);
        result.length = (size_t) size.QuadPart;
    }

    if (should_read) {
        unsigned long bytes_read = 0;
        result.contents = (char *) allocator_allocate(allocator, result.length + 1);
        result.is_valid = ReadFile(file_handle, result.contents, result.length + 1, &bytes_read, NULL);
        result.contents[result.length] = 0;
    }

    if (!result.is_valid) {
        allocator_release(allocator, result.contents);
    }

    CloseHandle(file_handle);
    return result;
}

#endif // defined(_WIN32)
