#include "../h/syscall_c.h"
#include "../h/syscall_abi.hpp"
#include "../lib/hw.h"

// ABI shims. "+r"(a0) keeps the syscall code and return value in a0;
// register-named locals let the compiler skip redundant mv's.

static inline uint64 ecall0(uint64 code) {
    register uint64 a0 asm("a0") = code;
    asm volatile ("ecall" : "+r"(a0) :: "memory");
    return a0;
}

static inline uint64 ecall1(uint64 code, uint64 a1_) {
    register uint64 a0 asm("a0") = code;
    register uint64 a1 asm("a1") = a1_;
    asm volatile ("ecall" : "+r"(a0) : "r"(a1) : "memory");
    return a0;
}

static inline uint64 ecall2(uint64 code, uint64 a1_, uint64 a2_) {
    register uint64 a0 asm("a0") = code;
    register uint64 a1 asm("a1") = a1_;
    register uint64 a2 asm("a2") = a2_;
    asm volatile ("ecall" : "+r"(a0) : "r"(a1), "r"(a2) : "memory");
    return a0;
}

static inline uint64 ecall4(uint64 code,
                            uint64 a1_, uint64 a2_,
                            uint64 a3_, uint64 a4_) {
    register uint64 a0 asm("a0") = code;
    register uint64 a1 asm("a1") = a1_;
    register uint64 a2 asm("a2") = a2_;
    register uint64 a3 asm("a3") = a3_;
    register uint64 a4 asm("a4") = a4_;
    asm volatile ("ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a2), "r"(a3), "r"(a4)
        : "memory");
    return a0;
}

// --- memory ---------------------------------------------------------------
// PDF p.8: ABI mem_alloc takes blocks, not bytes — convert here.
extern "C" void* mem_alloc(size_t size) {
    if (size == 0) return nullptr;
    size_t blocks = (size + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    return (void*)ecall1(SYS_MEM_ALLOC, blocks);
}

extern "C" int mem_free(void* ptr) {
    return (int)ecall1(SYS_MEM_FREE, (uint64)ptr);
}

// --- threads --------------------------------------------------------------
//
// PDF p.8: the ABI signature of thread_create takes an EXTRA arg — the pre-
// allocated stack. The C API is responsible for calling mem_alloc first and
// forwarding the stack pointer through ABI a4.
//
// We allocate DEFAULT_STACK_SIZE bytes, then pass the address of the LAST
// usable byte + 1 (top of stack; RISC-V stack grows down). Because mem_alloc
// hands us a 16-aligned block start and DEFAULT_STACK_SIZE = 4096 is a
// multiple of 16, the top is also 16-aligned — the required RISC-V sp
// invariant.

extern "C" int thread_create(thread_t* handle,
                             void (*start_routine)(void*),
                             void* arg) {
    if (!handle || !start_routine) return -1;

    void* stack = mem_alloc(DEFAULT_STACK_SIZE);
    if (!stack) return -2;

    void* stack_top = (void*)((unsigned char*)stack + DEFAULT_STACK_SIZE);

    int rc = (int)ecall4(SYS_THREAD_CREATE,
                        (uint64)handle,
                        (uint64)start_routine,
                        (uint64)arg,
                        (uint64)stack_top);
    if (rc != 0) {
        // kernel refused to build the TCB — reclaim the stack we allocated
        // so we don't leak it. Idempotent even if kernel took ownership
        // partially, because mem_free rejects bogus pointers.
        mem_free(stack);
    }
    return rc;
}

extern "C" int  thread_exit()     { return (int)ecall0(SYS_THREAD_EXIT); }
extern "C" void thread_dispatch() {          ecall0(SYS_THREAD_DISPATCH); }

// --- semaphores ----------------------------------------------------------
// PDF §"C API" p.7-8 codes 0x21-0x26. Handles are opaque (`sem_t` = void*).

extern "C" int sem_open(sem_t* handle, unsigned init) {
    return (int)ecall2(SYS_SEM_OPEN, (uint64)handle, (uint64)init);
}
extern "C" int sem_close(sem_t handle) {
    return (int)ecall1(SYS_SEM_CLOSE, (uint64)handle);
}
extern "C" int sem_wait(sem_t handle) {
    return (int)ecall1(SYS_SEM_WAIT, (uint64)handle);
}
extern "C" int sem_signal(sem_t handle) {
    return (int)ecall1(SYS_SEM_SIGNAL, (uint64)handle);
}
extern "C" int sem_wait_n(sem_t handle, unsigned n) {
    return (int)ecall2(SYS_SEM_WAIT_N, (uint64)handle, (uint64)n);
}
extern "C" int sem_signal_n(sem_t handle, unsigned n) {
    return (int)ecall2(SYS_SEM_SIGNAL_N, (uint64)handle, (uint64)n);
}

// --- console (skip-Task-4 fallback; kernel body from console.lib) --------
extern "C" void putc(char c) {          ecall1(SYS_PUTC, (uint64)(uint8)c); }
extern "C" char getc()       { return (char)ecall0(SYS_GETC); }
