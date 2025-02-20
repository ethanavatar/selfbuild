#define  SELF_BUILD_C_
#include "self_build/self_build.h"

#define  ALL_STDLIB_C_
#include "self_build/all_stdlib.h"

extern struct Build __declspec(dllexport) build(
    struct Build_Context *context,
    enum   Build_Kind     kind
) {
    struct Allocator *allocator = &context->allocator;
    struct Build lib = build_create(context, kind, "main");
    list_extend(&lib.sources, win32_list_files("src", "*.c", allocator));
    return lib;
}

static struct Build_Context_Options options = {
    .debug_info_kind = Debug_Info_Kind_Portable,
    .sanitizers      = Sanitizers_Kind_Undefined | Sanitizers_Kind_Integer,
};

int main(void) {
    struct Thread_Context tctx = { 0 };
    thread_context_init_and_equip(&tctx);

    struct Allocator   allocator = scratch_begin(NULL);
    struct Build_Context context = build_create_context(options, "selfbuild", "bin", &allocator);
    
    bootstrap(&context, "build.c", "build.exe");

    struct Build exe = build(&context, Build_Kind_Executable);
    build_module(&context, &exe);

    scratch_end(&allocator);
    thread_context_release();
    return 0;
}
