#include "../lib/hw.h"
#include "../h/riscv.hpp"
#include "../h/syscall_abi.hpp"
#include "../h/debug.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/TCB.hpp"
#include "../lib/console.h"

// MUST mirror src/trap_entry.S exactly. See that file for offset table.
struct TrapFrame {
    uint64 ra, t0, t1, t2, t3, t4, t5, t6;
    uint64 a0, a1, a2, a3, a4, a5, a6, a7;
    uint64 sepc, sstatus;                    // per-thread CSR snapshot
};

extern "C" void c_trap_handler(TrapFrame* f) {
    // Read cause from CSR (transient — same for every trap on this HART).
    // sepc/sstatus come from the frame, NOT the live CSRs — see trap_entry.S.
    uint64 cause = READ_CSR(scause);

    // --- async interrupts (MSB=1) ----------------------------------------
    // We enable SEIE (external) so console.lib's __getc unblocks; SSIE
    // (timer soft-int) stays masked. If either fires, we handle it here
    // WITHOUT advancing sepc — the interrupted thread resumes at the exact
    // instruction it was on.
    if (cause & SCAUSE_INT_BIT) {
        uint64 code = cause & ~SCAUSE_INT_BIT;
        if (code == 9) {
            // External IRQ — delegate to console.lib. It plic_claim()s,
            // dispatches to uartintr() if IRQ==10, and plic_complete()s.
            // Safe to call with interrupts masked (hardware cleared SIE on
            // trap entry) — in fact console_handler panics if SIE is set.
            console_handler();
        } else if (code == 1) {
            // Timer soft-int. We disabled SSIE, but be defensive: ack the
            // pending bit so it doesn't re-fire and swallow silently. We
            // don't do time-sharing.
            uint64 sip = READ_CSR(sip);
            WRITE_CSR(sip, sip & ~SIE_SSIE);
        }
        // else: unknown async IRQ — ignore; hardware will re-raise if it
        // actually matters. Do NOT advance sepc for async traps.
        return;
    }

    // --- synchronous exceptions from user code (MSB=0, not ecall) --------
    // If a thread does something illegal (executes a privileged instruction,
    // dereferences a bad pointer, etc.) we don't want to kill the whole
    // kernel — the offending thread deserves the blame. Kill it and yield.
    //
    // This matters for System_Mode_test (Test 7 in the OS12026 test suite),
    // which deliberately runs `csrr t6, sepc` from a user thread. That
    // instruction is privileged; the expected outcome is "process does NOT
    // terminate regularly" (per tests/uputstvo.txt). Killing the thread
    // means the busy-wait main loop never sees `finishedB = true`, so the
    // test's completion string never prints — which is exactly the grader's
    // expected failure mode.
    //
    // Caveat for our simplified design: because we run everything in S-mode,
    // hardware treats `csrr sepc` as legal and this branch does NOT fire for
    // that specific instruction. To get full Test 7 behavior we'd need a
    // proper U-mode transition on thread entry. This handler still catches
    // *other* illegal-instruction faults, which is worth having regardless.
    if (cause != SCAUSE_ECALL_U && cause != SCAUSE_ECALL_S) {
        // scause values (synchronous exceptions, MSB=0):
        //   1 = instruction access fault, 2 = illegal instr,
        //   5 = load access fault, 7 = store access fault,
        //   12/13/15 = page faults (n/a — no MMU used).
        kputs("\nthread trap: scause="); kputhex(cause);
        kputs(" sepc="); kputhex(f->sepc);
        kputs(" — killing offending thread\n");
        TCB::exit();
        return;                                   // unreachable
    }

    switch (f->a0) {
        case SYS_MEM_ALLOC:
            f->a0 = (uint64)MemoryAllocator::alloc_blocks(f->a1);
            break;
        case SYS_MEM_FREE:
            f->a0 = (uint64)MemoryAllocator::free((void*)f->a1);
            break;

        // --- Task 2 ------------------------------------------------------
        case SYS_THREAD_CREATE: {
            // ABI: a1=handle*, a2=body, a3=arg, a4=stack_top.
            TCB** handle = (TCB**)f->a1;
            void  (*body)(void*) = (void(*)(void*))f->a2;
            void*  arg        = (void*)f->a3;
            void*  stack_top  = (void*)f->a4;
            f->a0 = (uint64)TCB::create(handle, body, arg, stack_top);
            break;
        }
        case SYS_THREAD_EXIT: {
            // Advance THIS thread's saved sepc BEFORE we yield away, so when
            // (much later) something restores us for cleanup — actually never,
            // since state is FINISHED — no confusion arises. More importantly,
            // when we context-switch inside TCB::exit, the callee-saved regs
            // and stack of *this* trap frame stay on our (soon-abandoned) stack.
            // We just never come back to it, which is fine.
            f->sepc += 4;
            TCB::exit();
            // Unreachable.
            f->a0 = 0;
            return;
        }
        // --- Task 4 fallback via console.lib -----------------------------
        // We're skipping Task 4 proper (async / time_sleep / PeriodicThread),
        // but the PDF requires all three interface layers for putc/getc, and
        // console.lib provides the kernel body.
        case SYS_PUTC:
            __putc((char)f->a1);
            f->a0 = 0;
            break;
        case SYS_GETC:
            // Sign-extend so EOF (-1 as char) becomes -1 as int on the caller.
            f->a0 = (uint64)(long)(signed char)__getc();
            break;

        case SYS_THREAD_DISPATCH:
            // Advance sepc first (before the switch) so that when we context-
            // switch back into this thread and eventually sret, we resume
            // past the ecall — not on top of it.
            f->sepc += 4;
            TCB::dispatch();
            // NOTE: fall through to the common "sepc += 4" at the bottom
            // would double-advance. Return early instead.
            return;

        default:
            f->a0 = (uint64)-1;
    }

    // Advance past the ecall instruction. Note this modifies the FRAME copy,
    // not the CSR — the S-file restores from the frame right before sret.
    f->sepc += 4;
}
