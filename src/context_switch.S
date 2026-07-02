# context_switch(old_ctx*, new_ctx*)
#   a0 = pointer to 14x uint64 array to save into
#   a1 = pointer to 14x uint64 array to restore from
#
# Layout (matches TCB::CTX_* constants):
#   0*8: ra
#   1*8: sp
#   2*8..13*8: s0..s11
#
# Why only these 14? RISC-V's calling convention says callee-saved regs
# (ra, sp, s0-s11) survive across function calls, while caller-saved
# (t0-t6, a0-a7) do not — the compiler already spills any live ones to
# the stack at the call site. Since context_switch is invoked as a
# regular C function from the C dispatcher (thread_dispatch, thread_exit,
# sem_wait), the caller-saved set is either dead or already on the caller's
# stack. Saving the callee-saved set into the TCB is enough to fully
# capture "where this thread was and what to restore when it comes back".
#
# For Task 4 (async preemption), a timer IRQ could land anywhere — including
# right before a caller-saved reg gets used. That would require a full
# 31-GPR save; but we're not doing Task 4.

.section .text
.globl context_switch
.align 4

context_switch:
    # ---- save current thread's context into *a0 ----
    sd  ra,   0*8(a0)
    sd  sp,   1*8(a0)
    sd  s0,   2*8(a0)
    sd  s1,   3*8(a0)
    sd  s2,   4*8(a0)
    sd  s3,   5*8(a0)
    sd  s4,   6*8(a0)
    sd  s5,   7*8(a0)
    sd  s6,   8*8(a0)
    sd  s7,   9*8(a0)
    sd  s8,  10*8(a0)
    sd  s9,  11*8(a0)
    sd  s10, 12*8(a0)
    sd  s11, 13*8(a0)

    # ---- load next thread's context from *a1 ----
    ld  ra,   0*8(a1)
    ld  sp,   1*8(a1)
    ld  s0,   2*8(a1)
    ld  s1,   3*8(a1)
    ld  s2,   4*8(a1)
    ld  s3,   5*8(a1)
    ld  s4,   6*8(a1)
    ld  s5,   7*8(a1)
    ld  s6,   8*8(a1)
    ld  s7,   9*8(a1)
    ld  s8,  10*8(a1)
    ld  s9,  11*8(a1)
    ld  s10, 12*8(a1)
    ld  s11, 13*8(a1)

    # ret jumps to the loaded ra — either the original caller's return
    # point (for an established thread) or the wrapper's entry point
    # (for a brand-new thread seeded by TCB::create).
    ret
