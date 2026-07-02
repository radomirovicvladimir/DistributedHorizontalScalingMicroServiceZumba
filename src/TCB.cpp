#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/debug.hpp"

// --- static state ---------------------------------------------------------

TCB* TCB::running = nullptr;
TCB* TCB::mainTCB = nullptr;
TCB* TCB::idleTCB = nullptr;

// Exported so Scheduler::switch_to_next() can compare against it without a
// friend declaration. Same value as TCB::idleTCB — a plain global for the
// switch site's convenience.
TCB* g_idle = nullptr;

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
// Reached ONLY when context_switch first loads a new TCB's context. At that
// point:
//   * running == the new TCB (Scheduler::switch_to_next() set it before the
//     context_switch call),
//   * ra was seeded to &body_wrapper by TCB::create,
//   * sp is the top of the new stack,
//   * s0..s11 are zero (or any value — we don't read them).
// So `ret` at the end of context_switch lands us right here, executing on the
// new thread's stack.

void TCB::body_wrapper() {
    TCB* self = TCB::running;
    self->body(self->arg);
    // If the user body returns normally, treat it as an implicit thread_exit.
    // (PDF §"Implementacija nekih zahtevanih funkcionalnosti", p.25.)
    TCB::exit();
}

// --- public lifecycle -----------------------------------------------------

int TCB::create(TCB** handle_out,
                void (*body)(void*),
                void* arg,
                void* stack_top) {
    if (!handle_out || !body || !stack_top) return -1;

    // Allocate the TCB itself from OUR allocator, direct (no ecall). PDF
    // p.21 explicitly bans `new` inside the kernel.
    const size_t need = (sizeof(TCB) + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    void* raw = MemoryAllocator::alloc_blocks(need);
    if (!raw) return -2;

    TCB* t = (TCB*)raw;
    // Manual field init — no non-trivial ctor, TCB is POD-ish.
    for (int i = 0; i < CTX_LEN; i++) t->context[i] = 0;
    t->body         = body;
    t->arg          = arg;
    t->next         = nullptr;
    t->state        = READY;
    t->is_kernel    = false;
    t->stack_bottom = (void*)((uchar*)stack_top - DEFAULT_STACK_SIZE);

    // Seed initial context. When context_switch loads this and does `ret`,
    // execution jumps to body_wrapper() with sp at the top of the stack.
    // RISC-V requires sp 16-aligned; the payload from mem_alloc is already
    // 16-aligned (see feedback-16b-align.md), so stack_top inherited that.
    t->context[CTX_RA] = (uint64)&TCB::body_wrapper;
    t->context[CTX_SP] = (uint64)stack_top;
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
    // mainTCB represents whoever called `main()` — namely the kernel's own
    // initial thread of control. It has no allocator-owned stack (it uses
    // main()'s stack, which was set up by hw.lib), so stack_bottom stays
    // null and free() is never called on it.
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

    // Build the idle thread. It DOES need its own stack because the first
    // time we switch into idle, `ra = &idle_body_wrapper`, `sp = stack_top`,
    // and it starts executing on that stack.
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
