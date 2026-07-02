#include "../lib/hw.h"
#include "../h/riscv.hpp"
#include "../h/syscall_abi.hpp"
#include "../h/debug.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"
#include "../h/Semaphore.hpp"
#include "../lib/console.h"

struct TrapFrame {
    uint64 ra, t0, t1, t2, t3, t4, t5, t6;
    uint64 a0, a1, a2, a3, a4, a5, a6, a7;
    uint64 sepc, sstatus;
};

extern "C" void c_trap_handler(TrapFrame* f) {

    uint64 cause = READ_CSR(scause);

    if (TCB::running) TCB::running->trap_frame = f;

    if (cause & SCAUSE_INT_BIT) {
        uint64 code = cause & ~SCAUSE_INT_BIT;
        if (code == 9) {

            console_handler();
        } else if (code == 1) {

            uint64 sip = READ_CSR(sip);
            WRITE_CSR(sip, sip & ~SIE_SSIE);
        }

        return;
    }

    if (cause != SCAUSE_ECALL_U && cause != SCAUSE_ECALL_S) {

        kputs("\nthread trap: scause="); kputhex(cause);
        kputs(" sepc="); kputhex(f->sepc);
        kputs(" — killing offending thread\n");
        TCB::exit();
        return;
    }

    switch (f->a0) {
        case SYS_MEM_ALLOC:
            f->a0 = (uint64)MemoryAllocator::alloc_blocks(f->a1);
            break;
        case SYS_MEM_FREE:
            f->a0 = (uint64)MemoryAllocator::free((void*)f->a1);
            break;

        case SYS_THREAD_CREATE: {

            TCB** handle = (TCB**)f->a1;
            void  (*body)(void*) = (void(*)(void*))f->a2;
            void*  arg        = (void*)f->a3;
            void*  stack_top  = (void*)f->a4;
            f->a0 = (uint64)TCB::create(handle, body, arg, stack_top);
            break;
        }
        case SYS_THREAD_EXIT: {

            f->sepc += 4;
            TCB::exit();

            f->a0 = 0;
            return;
        }

        case SYS_PUTC:
            __putc((char)f->a1);
            f->a0 = 0;
            break;
        case SYS_GETC:

            f->a0 = (uint64)(long)(signed char)__getc();
            break;

        case SYS_THREAD_DISPATCH:

            f->sepc += 4;
            TCB::dispatch();

            return;

        case SYS_SEM_OPEN: {

            KSemaphore** handle = (KSemaphore**)f->a1;
            unsigned init = (unsigned)f->a2;
            if (!handle) { f->a0 = (uint64)-1; break; }

            size_t blocks = (sizeof(KSemaphore) + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
            void* raw = MemoryAllocator::alloc_blocks(blocks);
            if (!raw) { f->a0 = (uint64)-1; break; }
            KSemaphore* s = new (raw) KSemaphore((int)init);
            *handle = s;
            f->a0 = 0;
            break;
        }
        case SYS_SEM_CLOSE: {
            KSemaphore* s = (KSemaphore*)f->a1;
            if (!s) { f->a0 = (uint64)-1; break; }
            s->close();
            s->~KSemaphore();
            MemoryAllocator::free(s);
            f->a0 = 0;
            break;
        }
        case SYS_SEM_WAIT:
        case SYS_SEM_WAIT_N: {
            KSemaphore* s = (KSemaphore*)f->a1;
            unsigned n = (f->a0 == SYS_SEM_WAIT_N) ? (unsigned)f->a2 : 1u;
            if (!s) { f->a0 = (uint64)-1; break; }
            int r = s->wait(n);
            if (r == 1) {

                f->sepc += 4;
                Scheduler::switch_to_next();
                return;
            }

            f->a0 = (uint64)(long)r;
            break;
        }
        case SYS_SEM_SIGNAL:
        case SYS_SEM_SIGNAL_N: {
            KSemaphore* s = (KSemaphore*)f->a1;
            unsigned n = (f->a0 == SYS_SEM_SIGNAL_N) ? (unsigned)f->a2 : 1u;
            if (!s) { f->a0 = (uint64)-1; break; }
            int r = s->signal(n);
            f->a0 = (uint64)(long)r;
            break;
        }

        default:
            f->a0 = (uint64)-1;
    }

    f->sepc += 4;
}
