#include "../lib/hw.h"
#include "../h/riscv.hpp"
#include "../h/syscall_abi.hpp"
#include "../h/debug.hpp"
#include "../h/MemoryAllocator.hpp"

// Frame layout MUST match trap_entry.S: 16 caller-saved registers, in order.
struct TrapFrame {
    uint64 ra, t0, t1, t2, t3, t4, t5, t6;
    uint64 a0, a1, a2, a3, a4, a5, a6, a7;
};

extern "C" void c_trap_handler(TrapFrame* f) {
    uint64 cause = READ_CSR(scause);
    uint64 pc    = READ_CSR(sepc);

    if (cause != SCAUSE_ECALL_U && cause != SCAUSE_ECALL_S) {
        kputs("\nunhandled trap: scause="); kputhex(cause);
        kputs(" sepc="); kputhex(pc); kputc('\n');
        kpanic("trap");
    }

    switch (f->a0) {
        case SYS_MEM_ALLOC:
            f->a0 = (uint64)MemoryAllocator::alloc_blocks(f->a1);
            break;
        case SYS_MEM_FREE:
            f->a0 = (uint64)MemoryAllocator::free((void*)f->a1);
            break;
        default:
            f->a0 = (uint64)-1;
    }
    WRITE_CSR(sepc, pc + 4);
}
