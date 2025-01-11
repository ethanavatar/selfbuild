#ifndef MEMORY_H
#define MEMORY_H

#define KiB(KIBS) (1024 * KIBS)
#define MiB(MIBS) (1024 * KiB(MIBS))

intptr_t align_backward(intptr_t address, intptr_t alignment) {
    intptr_t aligned_address = address & ~(alignment - 1);
    assert(aligned_address % alignment == 0);
    return aligned_address;
}

intptr_t align_forward(intptr_t address, intptr_t alignment) {
    return align_backward(address + (alignment - 1), alignment);
}

#endif // MEMORY_H
