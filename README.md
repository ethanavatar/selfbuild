# Self Build

A self-hosted build system + utilities library for C

Contents:
- [Disclaimers](#Disclaimers)
- [The Build System](#The-Build-System)
    - [Building Dependencies](#Building-Dependencies)
- [TODO](#TODO)
    - [self_build](#self_build)
    - [stdlib](#stdlib)

## Disclaimers

- Buggy and unreliable (Don't use it)
- Requires C23
- Requires Clang   (For now)
- Requires Windows (For now)

## The Build System

The build system is a part of the utilities library and it can be used to compile itself.
As an example, I'll walk through the "script" used to compile the library.
Or you can take a look at the full source ([build.c](./build.c)).

Both "self_build" and the "stdlib" need to be included, and for simplicity to begin bootstrapping, all of the source files should also be included. There are some [STB-Style](https://github.com/nothings/stb) macros to help with that.

```C
#define  SELF_BUILD_C_
#include "self_build/self_build.h"

#define  ALL_STDLIB_C_
#include "self_build/all_stdlib.h"
```

Next is a function to describe the build itself. This one is relatively simple.
It just includes all of the `*.c` files inside the respective directories.

```C
struct Build build(struct Build_Context *context, enum Build_Kind requested_kind) {
    struct Build lib = build_create(context, requested_kind, "self_build");
    
    list_extend(&lib.sources, win32_list_files("stdlib",     "*.c", &context->allocator));
    list_extend(&lib.sources, win32_list_files("self_build", "*.c", &context->allocator));

    return lib;
}
```

Finally is the "driver" of the build; the part that actually does the building.
Theres a lot of boilerplate stuff going on here, but the core of it is that it calls the `build` function from above, and passes its result into `build_module`.

```C
// Global build options
static struct Build_Context_Options options = {
    .debug_info_kind = Debug_Info_Kind_Portable,
};

int main(void) {
    // ----- Initialization -----
    struct Thread_Context tctx = { 0 };
    thread_context_init_and_equip(&tctx);

    struct Allocator   allocator = scratch_begin(NULL);
    struct Build_Context context = build_create_context(options, ".", "bin", &allocator);

    // Check for updates in this file and rerun with a new version
    bootstrap(&context, "build.c", "build.exe");


    // ----- Run the build -----
    struct Build module = build(&context, Build_Kind_Static_Library);
    build_module(&context, &tests_exe);


    // ----- Clean up -----
    scratch_end(&allocator);
    thread_context_release();

    return 0;
}
```

To turn ([build.c](./build.c)) into a build program, simply compile it with:
- `-I` pointed to the root directory of this repo so it can include the right header/source files
- `-std=c23` because I wrote it in C23

```Bash
$ clang build.c -o build.exe -std=c23 -I.
```

Then run the build.

```Bash
$ ./build.exe
```

And the resulting library will be at `bin/self_build.lib`

### Building Dependencies

This build system is able to build dependencies if you point it to a directory that contains a `build.c` file.

Suppose I have a library in a directory, `libhello`, and I want to add `libhello` as a dependency of this library. The build script for `libhello` looks like this:

```C
extern struct Build __declspec(dllexport) build(struct Build_Context *, enum Build_Kind);

struct Build build(struct Build_Context *context, enum Build_Kind requested_kind) {
    struct Build lib = build_create(context, requested_kind, "libhello");
    list_extend(&lib.sources, win32_list_files("libhello", "*.c", &context->allocator));
    return lib;
}
```

And in the parent script, I can add the dependency like so.

```C
struct Build build(struct Build_Context *context, enum Build_Kind requested_kind) {
    struct Build lib = build_create(context, requested_kind, "self_build");
    
    list_extend(&lib.sources, win32_list_files("stdlib",     "*.c", &context->allocator));
    list_extend(&lib.sources, win32_list_files("self_build", "*.c", &context->allocator));

    // Get the build description from `libhello/build.c`
    struct Build libhello = build_submodule(context, "libhello", Build_Kind_Static_Library);

    // Add `libhello` as a dependency
    add_dependency(&lib, libhello);

    return lib;
}
```

Notice that, in `libhello`, the `build` function has been exported. This is because adding dependencies as "submodules" will compile their `build.c` into a `.dll` rather than a `.exe`, and load its `build` function dynamically to acquire the build definition of the submodule. That way, everything goes through the same "driver" code inside of `main`.

## TODO

### self_build

- "Install" headers to the build directory like Zig
    - Implicitly add the build directory of a dependency to the includes of a dependant

### stdlib

- CSV Parser
    - @Ref: https://en.wikipedia.org/wiki/Comma-separated_values
    - @Ref: https://stackoverflow.com/questions/3349774/test-suite-for-csv-specification
    - @Ref: https://www.ietf.org/rfc/rfc4180.txt
