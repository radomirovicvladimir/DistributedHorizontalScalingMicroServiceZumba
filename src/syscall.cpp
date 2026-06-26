#include "../h/syscall_c.h"
#include "../h/syscall_abi.hpp"
#include "../lib/hw.h"

// ABI shim. "+r"(a0) keeps the syscall code and return value in a0;
// register-named locals let the compiler skip redundant mv's.
// More arities will be added as Tasks 2-4 land.

static inline uint64 ecall1(uint64 code, uint64 a1_) {
    register uint64 a0 asm("a0") = code;
    register uint64 a1 asm("a1") = a1_;
    asm volatile ("ecall" : "+r"(a0) : "r"(a1) : "memory");
    return a0;
}

// PDF p.8: ABI mem_alloc takes blocks, not bytes — convert here.
extern "C" void* mem_alloc(size_t size) {
    if (size == 0) return nullptr;
    size_t blocks = (size + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    return (void*)ecall1(SYS_MEM_ALLOC, blocks);
}

extern "C" int mem_free(void* ptr) {
    return (int)ecall1(SYS_MEM_FREE, (uint64)ptr);
}
