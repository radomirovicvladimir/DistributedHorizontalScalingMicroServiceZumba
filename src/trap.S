# Trap entry. One handler in stvec services ecall/exception/external IRQ.
# For Task 1 we only need ecall, and the kernel is non-preemptive, so we
# save only the caller-saved set (ra, t0-t6, a0-a7) — the C handler will
# preserve s0-s11 by ABI as usual. When async preemption arrives in Task 4
# we'll widen this save area to cover all 31 GPRs.
#
# stvec must be 4-byte aligned in direct mode (MODE bits = 0).

.section .text
.globl trap_entry
.align 4
trap_entry:
    addi sp, sp, -128

    sd ra,    0(sp)
    sd t0,    8(sp)
    sd t1,   16(sp)
    sd t2,   24(sp)
    sd t3,   32(sp)
    sd t4,   40(sp)
    sd t5,   48(sp)
    sd t6,   56(sp)
    sd a0,   64(sp)
    sd a1,   72(sp)
    sd a2,   80(sp)
    sd a3,   88(sp)
    sd a4,   96(sp)
    sd a5,  104(sp)
    sd a6,  112(sp)
    sd a7,  120(sp)

    mv a0, sp                 # pass &TrapFrame to the C dispatcher
    call c_trap_handler

    ld ra,    0(sp)
    ld t0,    8(sp)
    ld t1,   16(sp)
    ld t2,   24(sp)
    ld t3,   32(sp)
    ld t4,   40(sp)
    ld t5,   48(sp)
    ld t6,   56(sp)
    ld a0,   64(sp)           # return value lives here after the dispatcher writes it
    ld a1,   72(sp)
    ld a2,   80(sp)
    ld a3,   88(sp)
    ld a4,   96(sp)
    ld a5,  104(sp)
    ld a6,  112(sp)
    ld a7,  120(sp)

    addi sp, sp, 128
    sret
