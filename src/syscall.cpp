#include "../h/syscall_c.h"
#include "../h/syscall_abi.hpp"
#include "../lib/hw.h"

// C API: user-mode wrappers that issue ecall and return the kernel's reply.
// PDF p.8: the ABI mem_alloc takes size IN BLOCKS; this wrapper does the
// bytes->blocks conversion before the trap.
//
// Inline-asm idiom: use register-named locals and "+r"(a0) so the compiler
// keeps the syscall number and the return value in the same physical register.

extern "C" void* mem_alloc(size_t size) {
    if (size == 0) return nullptr;
    size_t blocks = (size + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;

    register uint64 a0 asm("a0") = SYS_MEM_ALLOC;
    register uint64 a1 asm("a1") = blocks;
    asm volatile ("ecall"
                  : "+r"(a0)
                  : "r"(a1)
                  : "memory");
    return (void*)a0;
}

extern "C" int mem_free(void* ptr) {
    register uint64 a0 asm("a0") = SYS_MEM_FREE;
    register uint64 a1 asm("a1") = (uint64)ptr;
    asm volatile ("ecall"
                  : "+r"(a0)
                  : "r"(a1)
                  : "memory");
    return (int)a0;
}
