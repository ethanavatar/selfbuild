#ifndef WIN32_PLATFORM_H
#define WIN32_PLATFORM_H

#include "stdlib/arena.h"
#include "stdlib/allocators.h"

enum File_Move_Flags {
    File_Move_Flags_None      = 0x0,
    File_Move_Flags_Overwrite = 0x1 << 0, // 0x1: MOVEFILE_REPLACE_EXISTING 
};

void *win32_load_library(const char *);
void  win32_free_library(void *);
void *win32_get_symbol_address(void *, const char *);

char *win32_get_current_directory(struct Allocator *);

void      win32_move_file(const char *, const char *, enum File_Move_Flags);
void      win32_copy_file(const char *, const char *);

bool      win32_dir_exists(const char *);
bool      win32_file_exists(const char *);
long long win32_get_file_last_modified_time(const char *);
int       win32_wait_for_command(const char *, const char *);
int       win32_wait_for_command_ex(char *, char *);
int       win32_wait_for_command_format(const char *, ...);
int       win32_wait_for_command_format_ex(char *, const char *, ...);
void      win32_create_directories(const char *);

struct String_List win32_list_files(char *directory, char *cwd, char *file_pattern, struct Allocator *allocator);
void win32_get_executable_dir(char *dir);

void *win32_create_file(char *file_path);
void  win32_close_file(void *handle);
void  win32_delete_file(char *file_path);

int64_t win32_get_system_timestamp(void);

#endif // WIN32_PLATFORM_H
