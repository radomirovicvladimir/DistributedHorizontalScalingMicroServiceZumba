#pragma once
#include "../lib/hw.h"

// Singleton memory allocator over [HEAP_START_ADDR, HEAP_END_ADDR).
// PDF §"Ključne apstrakcije" (p.21) names this class explicitly as singleton.
// Algorithm: K&R-style first-fit, address-sorted freelist, coalesce on free.
// Sizes are tracked in BLOCKS (units of MEM_BLOCK_SIZE = 64 B).
//
// Not thread-safe by design — the kernel is non-preemptive and runs the trap
// handler with interrupts masked, so syscalls are mutually exclusive.

class MemoryAllocator {
public:
    // One-time init. Call from main() before anything else allocates.
    static void  init();

    // bytes -> 16-aligned pointer or nullptr. Header sits at (ptr - 16).
    static void* alloc(size_t bytes);

    // 0 on success, -1 on a pointer that obviously isn't ours (out of bounds).
    // Coalesces with both neighbors in the freelist.
    static int   free(void* ptr);

    // Debug invariant walker. Panics on any violation. No-op in release builds.
    static void  check();

    // Stats for tests/debug.
    static size_t total_free_bytes();

    // Block size (re-exported for symmetry; same as MEM_BLOCK_SIZE from hw.h).
    static const size_t BLOCK = MEM_BLOCK_SIZE;

private:
    struct Header {
        Header* next;        // next free node (address-sorted), nullptr terminates
        size_t  blocks;      // size of THIS block, header included, in BLOCK units
    };
    // We rely on sizeof(Header) == 16 so payload after the header stays 16-aligned.
    static_assert(sizeof(Header) == 16, "Header must be exactly 16 bytes");

    static Header* freelist;

    MemoryAllocator() = delete;   // singleton: static-only, never instantiated
};
