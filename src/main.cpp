#include "../lib/hw.h"
#include "../h/debug.hpp"
#include "../h/riscv.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/syscall_c.h"
#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"

extern "C" void trap_entry();
void userMain();

static void userMain_wrapper(void*) {
    userMain();

}

extern "C" void main() {
    kputs("==== OS1 boot ====\n");
    kputs("HEAP "); kputhex((uint64)HEAP_START_ADDR);
    kputs(" .. ");  kputhex((uint64)HEAP_END_ADDR);
    kputs(" ("); kputdec((uint64)HEAP_END_ADDR - (uint64)HEAP_START_ADDR);
    kputs(" B)\n");

    MemoryAllocator::init();

    uint64 sie = READ_CSR(sie);
    WRITE_CSR(sie, (sie & ~SIE_SSIE) | SIE_SEIE);
    uint64 sst = READ_CSR(sstatus);
    WRITE_CSR(sstatus, sst | SSTATUS_SIE);

    WRITE_CSR(stvec, (uint64)&trap_entry);

    TCB::init();

    void* stack = mem_alloc(DEFAULT_STACK_SIZE);
    if (!stack) kpanic("main: OOM allocating userMain stack");
    void* stack_top = (uchar*)stack + DEFAULT_STACK_SIZE;

    TCB* userMainTCB = nullptr;
    if (TCB::create(&userMainTCB, userMain_wrapper, nullptr, stack_top) != 0) {
        kpanic("main: TCB::create for userMain failed");
    }

    while (!Scheduler::empty()) {
        thread_dispatch();
    }

    kputs("\n==== all user threads finished, halting ====\n");
    khalt();
}
