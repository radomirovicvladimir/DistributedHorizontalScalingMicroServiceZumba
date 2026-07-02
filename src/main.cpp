#include "../lib/hw.h"
#include "../h/debug.hpp"
#include "../h/riscv.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/syscall_c.h"
#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"

extern "C" void trap_entry();       // src/trap_entry.S
extern "C" void userMain();         // src/userMain.cpp — the test harness

// Kernel entry per PDF §"Odnos jezgra i korisničke aplikacije":
//   1. hw.lib boots CPU + PLIC + UART, calls main().
//   2. We init memory + traps + threads.
//   3. We spawn a thread running userMain (the app-side entry).
//   4. First thread_dispatch hands control to that thread.
//
// This replaces the previous in-tree self-test harness — the OS12026-shipped
// public tests in tests/ are what the grader runs, and they're wired up via
// tests/Threads_*_test.cpp + src/userMain.cpp.

static void userMain_wrapper(void*) {
    userMain();
    // userMain returned normally — we're done. The wrapper's fall-off will
    // call thread_exit implicitly (see TCB::body_wrapper in TCB.cpp).
}

extern "C" void main() {
    kputs("==== OS1 boot ====\n");
    kputs("HEAP "); kputhex((uint64)HEAP_START_ADDR);
    kputs(" .. ");  kputhex((uint64)HEAP_END_ADDR);
    kputs(" ("); kputdec((uint64)HEAP_END_ADDR - (uint64)HEAP_START_ADDR);
    kputs(" B)\n");

    // Task 1: bring up the heap.
    MemoryAllocator::init();

    // Install our trap handler in stvec (direct mode — MODE bits = 0).
    WRITE_CSR(stvec, (uint64)&trap_entry);

    // Task 2: bring up thread infrastructure. Creates mainTCB (running == us)
    // and idleTCB (always-ready fallback so switch_to_next never returns null).
    TCB::init();

    // Allocate a stack for the userMain thread. We use the same path a user
    // would — mem_alloc via ecall — so the stack ends up in the normal heap
    // and the layout matches what a user-spawned thread expects.
    void* stack = mem_alloc(DEFAULT_STACK_SIZE);
    if (!stack) kpanic("main: OOM allocating userMain stack");
    void* stack_top = (uchar*)stack + DEFAULT_STACK_SIZE;

    TCB* userMainTCB = nullptr;
    if (TCB::create(&userMainTCB, userMain_wrapper, nullptr, stack_top) != 0) {
        kpanic("main: TCB::create for userMain failed");
    }

    // Hand off. First dispatch swaps out of mainTCB into userMain (or idle
    // first, if the scheduler picks that — in practice FIFO gives us userMain
    // since idle isn't queued). When userMain's thread finishes, control
    // comes back here (mainTCB gets rescheduled), the ready queue drains,
    // and we fall through to khalt().
    while (!Scheduler::empty()) {
        thread_dispatch();
    }

    kputs("\n==== all user threads finished, halting ====\n");
    khalt();
}
