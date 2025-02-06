#ifndef ALL_STDLIB_H_
#define ALL_STDLIB_H_

#include "stdlib/win32_platform.h"
#include "stdlib/strings.h"
#include "stdlib/allocators.h"
#include "stdlib/arena.h"
#include "stdlib/thread_context.h"
#include "stdlib/managed_arena.h"
#include "stdlib/scratch_memory.h"
#include "stdlib/string_builder.h"
#include "stdlib/list.h"
#include "stdlib/libc_allocator.h"

#endif // ALL_STDLIB_H_

#ifdef ALL_STDLIB_C_

#include "stdlib/win32_platform.c"
#include "stdlib/strings.c"
#include "stdlib/allocators.c"
#include "stdlib/arena.c"
#include "stdlib/thread_context.c"
#include "stdlib/managed_arena.c"
#include "stdlib/scratch_memory.c"
#include "stdlib/string_builder.c"
#include "stdlib/list.c"
#include "stdlib/libc_allocator.c"

#undef ALL_STDLIB_C_
#endif
