#include "../lib/hw.h"
#include "../h/riscv.hpp"
#include "../h/syscall_abi.hpp"
#include "../h/debug.hpp"
#include "../h/MemoryAllocator.hpp"

// Frame layout MUST match trap_entry.S exactly: 16 caller-saved registers, in order.
struct TrapFrame {
    uint64 ra;
    uint64 t0, t1, t2, t3, t4, t5, t6;
    uint64 a0, a1, a2, a3, a4, a5, a6, a7;
};

// Dispatcher for ecall / exception / interrupt. For Task 1 only ecall is wired.
extern "C" void c_trap_handler(TrapFrame* f) {
    uint64 cause = READ_CSR(scause);
    uint64 pc    = READ_CSR(sepc);

    if (cause == SCAUSE_ECALL_U || cause == SCAUSE_ECALL_S) {
        // Syscall: dispatch by a0, result goes back in a0.
        switch (f->a0) {
            case SYS_MEM_ALLOC: {
                // ABI: a1 = size IN BLOCKS of payload (PDF p.8). The C-API
                // wrapper has already rounded user bytes up to whole blocks.
                // Call alloc_blocks directly — no blocks->bytes->blocks dance.
                f->a0 = (uint64)MemoryAllocator::alloc_blocks((size_t)f->a1);
                break;
            }
            case SYS_MEM_FREE: {
                f->a0 = (uint64)MemoryAllocator::free((void*)f->a1);
                break;
            }
            default:
                // Unknown syscall: signal error. (-1 in a0, sign-extended.)
                f->a0 = (uint64)-1;
                break;
        }
        // Step past the ecall instruction so sret resumes after it.
        WRITE_CSR(sepc, pc + 4);
        return;
    }

    // Anything else is unexpected during Task 1 — halt loudly.
    kputs("\nunhandled trap: scause="); kputhex(cause);
    kputs(" sepc=");                    kputhex(pc);
    kputc('\n');
    kpanic("trap");
}
