#define  SELF_BUILD_C_
#include "self_build/self_build.h"

#define  ALL_STDLIB_C_
#include "self_build/all_stdlib.h"

extern struct Build __declspec(dllexport) build(
    struct Build_Context *context,
    enum   Build_Kind     kind
) {
    struct Allocator *allocator = &context->allocator;
    struct Build lib = build_create(context, kind, "self_build");
    list_extend(&lib.sources, win32_list_files("stdlib",     "*.c", allocator));
    list_extend(&lib.sources, win32_list_files("self_build", "*.c", allocator));
    return lib;
}

struct Build build_windowing(
    struct Build_Context *context,
    enum   Build_Kind     kind
) {
    struct Allocator *allocator = &context->allocator;
    struct Build lib = build_create(context, kind, "windowing");
    list_extend(&lib.sources, win32_list_files("windowing", "*.c", allocator));

    build_add_system_library(&lib, "opengl32");
    build_add_system_library(&lib, "gdi32");

    // @TODO: Add this as a context option
    list_append(&lib.link_flags, cstring_to_string("-fsanitize=address,undefined", allocator));

    return lib;
}

struct Build build_tests(
    struct Build_Context *context,
    enum   Build_Kind     kind
) {
    struct Allocator *allocator = &context->allocator;
    struct Build test_exe = build_create(context, kind, "tests");
    list_extend(&test_exe.sources, win32_list_files("tests", "*.c", allocator));
    return test_exe;
}

struct Build build_testbed(
    struct Build_Context *context,
    enum   Build_Kind     kind
) {
    struct Allocator *allocator = &context->allocator;
    struct Build exe = build_create(context, kind, "testbed");
    build_add_include_path(&exe, "cglm/include");
    list_extend(&exe.sources,  win32_list_files("testbed", "*.c", allocator));

    list_append(&exe.link_flags, cstring_to_string("-fsanitize=address,undefined", allocator));

    return exe;
}

static struct Build_Context_Options options = {
    .debug_info_kind = Debug_Info_Kind_Portable,
};

int main(void) {
    struct Thread_Context tctx = { 0 };
    thread_context_init_and_equip(&tctx);

    struct Allocator   allocator = scratch_begin(NULL);
    struct Build_Context context = build_create_context(options, ".", "bin", &allocator);
    
    bootstrap(&context, "build.c", "build.exe");

    struct Build stdlib    = build           (&context, Build_Kind_Static_Library);
    struct Build tests     = build_tests     (&context, Build_Kind_Executable);
    struct Build windowing = build_windowing (&context, Build_Kind_Shared_Library);
    struct Build testbed   = build_testbed   (&context, Build_Kind_Executable);

    add_dependency(&tests, stdlib);
    build_module(&context, &tests);

    add_dependency(&testbed, stdlib);
    add_dependency(&testbed, windowing);
    build_module(&context, &testbed);

    scratch_end(&allocator);
    thread_context_release();
    return 0;
}
