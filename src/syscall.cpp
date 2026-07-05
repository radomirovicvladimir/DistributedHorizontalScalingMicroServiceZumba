#include "../h/syscall_c.h"
#include "../h/syscall_abi.hpp"
#include "../lib/hw.h"

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

extern "C" void* mem_alloc(size_t size) {
    if (size == 0) return nullptr;
    size_t blocks = (size + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    return (void*)ecall1(SYS_MEM_ALLOC, blocks);
}

extern "C" int mem_free(void* ptr) {
    return (int)ecall1(SYS_MEM_FREE, (uint64)ptr);
}

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

        mem_free(stack);
    }
    return rc;
}

extern "C" int  thread_exit()     { return (int)ecall0(SYS_THREAD_EXIT); }
extern "C" void thread_dispatch() {          ecall0(SYS_THREAD_DISPATCH); }
extern "C" int  getThreadId()     { return (int)ecall0(SYS_THREAD_GET_ID); }
extern "C" int  setMaximumThreads(int n) {
    return (int)ecall1(SYS_SET_MAX_THREADS, (uint64)n);
}

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

extern "C" void putc(char c) {          ecall1(SYS_PUTC, (uint64)(uint8)c); }
extern "C" char getc()       { return (char)ecall0(SYS_GETC); }
