#pragma once
#include "../lib/hw.h"

// First-fit allocator over [HEAP_START_ADDR, HEAP_END_ADDR).
// Sorted, coalescing freelist. Sizes tracked in MEM_BLOCK_SIZE blocks.
// Not thread-safe — relies on non-preemptive kernel + masked interrupts.
class MemoryAllocator {
public:
    static void   init();
    static void*  alloc(size_t bytes);            // user-facing, rounds up
    static void*  alloc_blocks(size_t payload);   // ABI-facing, payload in blocks
    static int    free(void* ptr);                // 0 ok, -1 bogus/double-free
    static void   check();                        // panics on broken freelist
    static size_t free_bytes();                   // for tests

private:
    struct Node { Node* next; size_t blocks; };   // 16B; doubles as in-use header
    static_assert(sizeof(Node) == 16, "Node must be 16 bytes");
    static Node* head;

    MemoryAllocator() = delete;
};
