#pragma once
#include "../lib/hw.h"

// Thread Control Block. Each user thread owns exactly one TCB, allocated
// off our own MemoryAllocator (never `new` — see feedback-kernel-no-new.md).
//
// The kernel runs on the current thread's stack: syscalls, yields, and any
// nested C calls happen in the TCB's kernel-owned buffer. This is the PDF's
// "per-thread kernel stack" model (§"Stek na kom se izvršava kôd jezgra",
// second bullet). yield() swaps context between TCBs at synchronous points.
//
// Context = the 14 RISC-V callee-saved registers, saved by yield() at each
// switch. Caller-saved regs live on the stack across a normal function call,
// so we don't touch them — this is the whole reason Task 2 works with a sync
// context switch alone. Task 4 (async preemption) would need to save all 31.

class TCB {
public:
    enum State { READY, RUNNING, FINISHED };

    // Layout: yield() indexes into context[] with these constants.
    // Keep in sync with context_switch.S (CTX_RA=0, CTX_SP=1, CTX_S0..S11=2..13).
    static const int CTX_RA = 0;
    static const int CTX_SP = 1;
    static const int CTX_S0 = 2;   // s0..s11 fill 2..13
    static const int CTX_LEN = 14;

    // Public state (accessed from trap dispatcher / wrapper / scheduler).
    uint64  context[CTX_LEN];
    void*   stack_bottom;          // MemoryAllocator::free() takes this
    void  (*body)(void*);
    void*   arg;
    TCB*    next;                  // ready-queue link
    State   state;
    bool    is_kernel;             // reserved — unused in Task 2 (kept for Task 4)

    // --- lifecycle -------------------------------------------------------

    // Kernel-side of SYS_THREAD_CREATE. `stack_top` is a pointer to the LAST
    // (highest) usable byte + 1 of a pre-allocated stack of DEFAULT_STACK_SIZE
    // bytes. Returns 0 on success (writes TCB* into *handle_out), <0 on error.
    static int create(TCB** handle_out,
                      void (*body)(void*),
                      void* arg,
                      void* stack_top);

    // Kernel-side of SYS_THREAD_EXIT. Marks running thread FINISHED and yields
    // to the next ready thread. Never returns to caller.
    __attribute__((noreturn)) static void exit();

    // Kernel-side of SYS_THREAD_DISPATCH. If a ready thread exists, puts
    // running back on the ready queue and switches to that next thread.
    static void dispatch();

    // Kernel init: builds the pseudo-TCB for main() (so `running` is never
    // null) and the idle thread (so `Scheduler::get()` never returns null
    // while any user thread lives).
    static void init();

private:
    // Trampoline installed as the initial `ra` of every new TCB. Fetches
    // running->body/arg, calls it, then thread_exit. NEVER returns to caller
    // because ra was seeded by TCB::create, not saved by a real caller.
    static void body_wrapper();

    // Lays down a synthetic 144-byte trap-return frame at the top of a new
    // thread's stack for the U-mode first-entry path (see TCB.cpp).
    // Returns the pointer to install as context[CTX_SP].
    static void* seed_initial_frame(void* stack_top, bool user_mode);

    // Bookkeeping.
    static TCB* mainTCB;        // no stack of its own (uses whatever main() had)
    static TCB* idleTCB;
    static void idle_body(void*);

    friend class Scheduler;
public:
    static TCB* running;        // exported for the trap dispatcher / wrapper
};
