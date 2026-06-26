#include "../h/syscall_c.h"
#include "../h/syscall_abi.hpp"
#include "../lib/hw.h"

// ---- ABI shim ------------------------------------------------------------
//
// One inline helper per arity. The "+r"(a0) constraint keeps the syscall
// code and the return value in the same physical register (a0), and the
// register-named locals let the compiler skip the `mv` when the value
// is already in the right register.
//
// Style borrowed from OS12026; constraint discipline is ours.

static inline uint64 ecall0(uint64 code) {
    register uint64 a0 asm("a0") = code;
    asm volatile ("ecall" : "+r"(a0) :: "memory");
    return a0;
}

static inline uint64 ecall1(uint64 code, uint64 _a1) {
    register uint64 a0 asm("a0") = code;
    register uint64 a1 asm("a1") = _a1;
    asm volatile ("ecall" : "+r"(a0) : "r"(a1) : "memory");
    return a0;
}

static inline uint64 ecall2(uint64 code, uint64 _a1, uint64 _a2) {
    register uint64 a0 asm("a0") = code;
    register uint64 a1 asm("a1") = _a1;
    register uint64 a2 asm("a2") = _a2;
    asm volatile ("ecall" : "+r"(a0) : "r"(a1), "r"(a2) : "memory");
    return a0;
}

// 5-arg variant: code + 4 args (a1..a4). Used by thread_create in Task 2.
static inline uint64 ecall5(uint64 code, uint64 _a1, uint64 _a2,
                             uint64 _a3, uint64 _a4) {
    register uint64 a0 asm("a0") = code;
    register uint64 a1 asm("a1") = _a1;
    register uint64 a2 asm("a2") = _a2;
    register uint64 a3 asm("a3") = _a3;
    register uint64 a4 asm("a4") = _a4;
    asm volatile ("ecall"
                  : "+r"(a0)
                  : "r"(a1), "r"(a2), "r"(a3), "r"(a4)
                  : "memory");
    return a0;
}

// ---- C API wrappers ------------------------------------------------------
//
// PDF p.8: the ABI mem_alloc takes size IN BLOCKS. We do the bytes->blocks
// conversion here so the kernel side can deal in blocks throughout.

extern "C" void* mem_alloc(size_t size) {
    if (size == 0) return nullptr;
    size_t blocks = (size + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    return (void*)ecall1(SYS_MEM_ALLOC, blocks);
}

extern "C" int mem_free(void* ptr) {
    return (int)ecall1(SYS_MEM_FREE, (uint64)ptr);
}
