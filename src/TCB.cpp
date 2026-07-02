#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/riscv.hpp"
#include "../h/debug.hpp"

// --- static state ---------------------------------------------------------

TCB* TCB::running = nullptr;
TCB* TCB::mainTCB = nullptr;
TCB* TCB::idleTCB = nullptr;

// Exported so Scheduler::switch_to_next() can compare against it without a
// friend declaration. Same value as TCB::idleTCB — a plain global for the
// switch site's convenience.
TCB* g_idle = nullptr;

// The tail of trap_entry.S — pops a 144-byte trap frame off sp and sret's.
// A fresh USER thread seeds its context to land here, so the first "switch
// in" pops a synthetic frame and enters U-mode via sret.
extern "C" void trap_return_tail();

// --- synthetic first-entry trap frame ------------------------------------
//
// Layout MUST match struct TrapFrame in src/trap.cpp and src/trap_entry.S:
//     0..15  : ra, t0-t6, a0-a7  (16 * 8 = 128 bytes)
//     16     : sepc
//     17     : sstatus
// Total: 18 slots * 8 = 144 bytes.
//
// Fields we care about for a fresh thread:
//   * ra   — value doesn't matter; body_wrapper never `ret`s (it thread_exits).
//   * sepc — set to body_wrapper. After sret, PC = sepc → executes wrapper.
//   * sstatus:
//       SPP  (bit 8) = 0 for user, 1 for kernel — determines mode after sret.
//       SPIE (bit 5) = 1 so interrupts get re-enabled once we're past sret,
//         which is required so console.lib's __getc unblocks on UART IRQ.
//   * Every other GPR slot = 0.
struct InitialFrame {
    uint64 gpr[16];        // ra, t0-t6, a0-a7
    uint64 sepc;
    uint64 sstatus;
};

// Build a synthetic frame at (stack_top - sizeof(InitialFrame)). Returns the
// pointer to install as the fresh thread's context[CTX_SP]. body_wrapper will
// run with sp at (stack_top - sizeof(InitialFrame)), which is 16-aligned as
// long as stack_top is 16-aligned (144 bytes IS a multiple of 16 → yes).
void* TCB::seed_initial_frame(void* stack_top, bool user_mode) {
    uchar* p = (uchar*)stack_top - sizeof(InitialFrame);
    InitialFrame* f = (InitialFrame*)p;

    for (int i = 0; i < 16; i++) f->gpr[i] = 0;
    f->sepc = (uint64)&TCB::body_wrapper;

    // Compose sstatus. SPP is bit 8; SPIE is bit 5. Everything else zero —
    // we don't care about FS/XS state (no floating point, no vector).
    uint64 sst = SSTATUS_SPIE;
    if (!user_mode) sst |= SSTATUS_SPP;      // stay in S-mode on sret
    f->sstatus = sst;

    return p;
}

// --- idle thread ----------------------------------------------------------

void TCB::idle_body(void* /*arg*/) {
    // Non-preemptive kernel: no interrupts land here, so `wfi` isn't strictly
    // necessary — but it's the polite thing to do (lets QEMU sleep the vCPU
    // instead of spinning at 100%). We stay here until the only user thread
    // exits and main() drops through to khalt().
    for (;;) {
        asm volatile("wfi");
    }
}

// --- trampoline for freshly-created threads ------------------------------
//
// For a KERNEL thread (idle, or any future kernel-mode helper), this is
// reached via context_switch → `ret` into &body_wrapper. sp points at the
// top of the new thread's stack; s0..s11 are zero. We're in S-mode.
//
// For a USER thread, this is reached via trap_return_tail → `sret` with
// sstatus.SPP=0. We're in U-mode. sp still points at the top of the stack
// (the synthetic frame we popped in the epilogue has been discarded).
//
// Either way, we fetch body/arg from the TCB (never from registers — the
// registers were either zero or restored from a frame). If body returns
// normally we do an implicit thread_exit via the user-facing C API — that
// ecall's into the trap handler which calls TCB::exit(). This ensures a
// U-mode thread's exit goes through the proper syscall boundary rather than
// executing kernel code directly in user mode.
extern "C" int thread_exit();   // user-side C API in src/syscall.cpp
void TCB::body_wrapper() {
    TCB* self = TCB::running;
    self->body(self->arg);
    thread_exit();              // ecall → trap → TCB::exit() → never returns
    // If for some reason thread_exit did return (it shouldn't), fall into
    // a spin so we don't corrupt anything.
    for (;;) {}
}

// --- public lifecycle -----------------------------------------------------

int TCB::create(TCB** handle_out,
                void (*body)(void*),
                void* arg,
                void* stack_top) {
    if (!handle_out || !body || !stack_top) return -1;

    // Allocate the TCB itself from OUR allocator, direct (no ecall).
    const size_t need = (sizeof(TCB) + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    void* raw = MemoryAllocator::alloc_blocks(need);
    if (!raw) return -2;

    TCB* t = (TCB*)raw;
    for (int i = 0; i < CTX_LEN; i++) t->context[i] = 0;
    t->body         = body;
    t->arg          = arg;
    t->next         = nullptr;
    t->state        = READY;
    t->is_kernel    = false;                         // user thread
    t->stack_bottom = (void*)((uchar*)stack_top - DEFAULT_STACK_SIZE);

    // Build a synthetic trap-return frame at the top of the thread's stack.
    // First switch into this TCB will run through trap_return_tail, which
    // pops the frame and `sret`s into U-mode at body_wrapper.
    void* frame_sp = TCB::seed_initial_frame(stack_top, /*user_mode=*/true);

    t->context[CTX_RA] = (uint64)&trap_return_tail;
    t->context[CTX_SP] = (uint64)frame_sp;
    // s0..s11 already zeroed.

    *handle_out = t;
    Scheduler::put(t);
    return 0;
}

void TCB::exit() {
    TCB::running->state = FINISHED;

    // Hand control to the next thread. We'll never come back to this stack
    // frame — context_switch inside switch_to_next saves our (now-dead) ctx,
    // but nothing will ever restore it because we're FINISHED.
    Scheduler::switch_to_next();

    // Actually unreachable. If we ever get here, something's badly wrong.
    kpanic("thread_exit returned");
    for (;;) {}   // silence noreturn warning if kpanic isn't inlined
}

void TCB::dispatch() {
    // If nothing else is ready and idle isn't relevant (we're not exiting),
    // just stay running — no need to swap to ourselves via idle.
    if (Scheduler::empty()) return;

    Scheduler::switch_to_next();
}

// --- kernel init ----------------------------------------------------------

void TCB::init() {
    // mainTCB represents whoever called `main()` — the kernel's initial
    // thread of control. Runs on hw.lib's boot stack in S-mode. context is
    // captured on-the-fly by the first context_switch — no synthetic frame
    // needed. When restored later, context_switch's `ret` returns to just
    // after the call site inside Scheduler::switch_to_next, still in S-mode.
    const size_t tcb_blocks = (sizeof(TCB) + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;

    void* main_raw = MemoryAllocator::alloc_blocks(tcb_blocks);
    if (!main_raw) kpanic("TCB::init: OOM allocating mainTCB");
    mainTCB = (TCB*)main_raw;
    for (int i = 0; i < CTX_LEN; i++) mainTCB->context[i] = 0;
    mainTCB->body         = nullptr;
    mainTCB->arg          = nullptr;
    mainTCB->next         = nullptr;
    mainTCB->state        = RUNNING;
    mainTCB->is_kernel    = true;
    mainTCB->stack_bottom = nullptr;

    running = mainTCB;

    // Build the idle thread. It stays in S-mode (uses `wfi`, which is
    // S-only), so we use the plain body_wrapper entry — context_switch
    // `ret`s straight into it in S-mode. No synthetic frame required.
    void* idle_stack = MemoryAllocator::alloc(DEFAULT_STACK_SIZE);
    if (!idle_stack) kpanic("TCB::init: OOM allocating idle stack");
    void* idle_stack_top = (uchar*)idle_stack + DEFAULT_STACK_SIZE;

    void* idle_raw = MemoryAllocator::alloc_blocks(tcb_blocks);
    if (!idle_raw) kpanic("TCB::init: OOM allocating idleTCB");
    idleTCB = (TCB*)idle_raw;
    for (int i = 0; i < CTX_LEN; i++) idleTCB->context[i] = 0;
    idleTCB->body         = &TCB::idle_body;
    idleTCB->arg          = nullptr;
    idleTCB->next         = nullptr;
    idleTCB->state        = READY;
    idleTCB->is_kernel    = true;
    idleTCB->stack_bottom = idle_stack;
    idleTCB->context[CTX_RA] = (uint64)&TCB::body_wrapper;
    idleTCB->context[CTX_SP] = (uint64)idle_stack_top;

    g_idle = idleTCB;
    // Not put on ready queue — it's the fallback in Scheduler::switch_to_next.
}
