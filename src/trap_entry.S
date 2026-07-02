# Trap entry. One handler in stvec services ecall/exception/external IRQ.
# Task 2 introduces context switches from within the C dispatcher — so we
# must save enough state on the CURRENT thread's stack to fully resume it
# even if a different thread services intermediate traps in the meantime.
# Specifically: sepc + sstatus need to be per-thread (they're CSRs, so a
# naive read/write in trap.cpp would clobber another thread's saved PC).
#
# Frame layout — MUST match struct TrapFrame in src/trap.cpp exactly:
#     0..15   : 16 caller-saved GPRs (ra, t0-t6, a0-a7)  — 128 bytes
#     16..17  : sepc, sstatus                            — 16 bytes
# Total: 144 bytes.
#
# stvec must be 4-byte aligned in direct mode (MODE bits = 0).

.section .text
.globl trap_entry
.align 4
trap_entry:
    addi sp, sp, -144

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

    # Snapshot sepc and sstatus into the frame. If the C dispatcher context-
    # switches away and back, the CSRs will have been transiently owned by
    # another thread — but our frame keeps the original values so `sret`
    # returns to *our* PC with *our* privilege bits.
    csrr t0, sepc
    sd   t0, 128(sp)
    csrr t0, sstatus
    sd   t0, 136(sp)

    mv a0, sp                 # pass &TrapFrame to the C dispatcher
    call c_trap_handler

    # Restore possibly-modified CSRs from the frame. (c_trap_handler writes
    # f->sepc += 4 for ecall; other trap kinds may leave it untouched.)
    ld   t0, 128(sp)
    csrw sepc, t0
    ld   t0, 136(sp)
    csrw sstatus, t0

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

    addi sp, sp, 144
    sret
