#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/riscv.hpp"
#include "../h/debug.hpp"

TCB* TCB::running = nullptr;
TCB* TCB::mainTCB = nullptr;
TCB* TCB::idleTCB = nullptr;

TCB* g_idle = nullptr;

size_t TCB::next_id            = 0;
int    TCB::max_user_threads   = 5;
int    TCB::active_user_threads = 0;
TCB*   TCB::pending_head       = nullptr;
TCB*   TCB::pending_tail       = nullptr;

extern "C" void trap_return_tail();

struct InitialFrame {
    uint64 gpr[16];
    uint64 sepc;
    uint64 sstatus;
};

void* TCB::seed_initial_frame(void* stack_top, bool user_mode) {
    uchar* p = (uchar*)stack_top - sizeof(InitialFrame);
    InitialFrame* f = (InitialFrame*)p;

    for (int i = 0; i < 16; i++) f->gpr[i] = 0;
    f->sepc = (uint64)&TCB::body_wrapper;

    uint64 sst = SSTATUS_SPIE;
    if (!user_mode) sst |= SSTATUS_SPP;
    f->sstatus = sst;

    return p;
}

void TCB::idle_body(void*) {
    for (;;) {
        asm volatile("wfi");
    }
}

extern "C" int thread_exit();
void TCB::body_wrapper() {
    TCB* self = TCB::running;
    self->body(self->arg);
    thread_exit();

    for (;;) {}
}

int TCB::create(TCB** handle_out,
                void (*body)(void*),
                void* arg,
                void* stack_top) {
    if (!handle_out || !body || !stack_top) return -1;

    const size_t need = (sizeof(TCB) + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    void* raw = MemoryAllocator::alloc_blocks(need);
    if (!raw) return -2;

    TCB* t = (TCB*)raw;
    for (int i = 0; i < CTX_LEN; i++) t->context[i] = 0;
    t->body         = body;
    t->arg          = arg;
    t->next         = nullptr;
    t->state        = READY;
    t->is_kernel    = false;
    t->stack_bottom = (void*)((uchar*)stack_top - DEFAULT_STACK_SIZE);
    t->trap_frame   = nullptr;
    t->wait_n       = 0;
    t->sem_result   = 0;
    t->id           = ++next_id;

    void* frame_sp = TCB::seed_initial_frame(stack_top, true);

    t->context[CTX_RA] = (uint64)&trap_return_tail;
    t->context[CTX_SP] = (uint64)frame_sp;

    *handle_out = t;

    if (active_user_threads < max_user_threads) {
        active_user_threads++;
        Scheduler::put(t);
    } else {
        t->state = PENDING_QUOTA;
        t->next  = nullptr;
        if (!pending_head) {
            pending_head = pending_tail = t;
        } else {
            pending_tail->next = t;
            pending_tail       = t;
        }
    }
    return 0;
}

void TCB::admit_from_pending() {
    if (!pending_head) return;
    TCB* t = pending_head;
    pending_head = t->next;
    if (!pending_head) pending_tail = nullptr;
    t->next  = nullptr;
    t->state = READY;
    active_user_threads++;
    Scheduler::put(t);
}

void TCB::exit() {
    TCB* me = TCB::running;
    me->state = FINISHED;

    if (!me->is_kernel) {
        if (active_user_threads > 0) active_user_threads--;
        admit_from_pending();
    }

    Scheduler::switch_to_next();

    kpanic("thread_exit returned");
    for (;;) {}
}

void TCB::dispatch() {
    if (Scheduler::empty()) return;
    Scheduler::switch_to_next();
}

size_t TCB::get_thread_id() {
    size_t id = TCB::running->id;
    Scheduler::switch_to_next();
    return id;
}

void TCB::set_maximum_threads(int n) {
    if (n < 1) n = 1;
    max_user_threads = n;
    while (active_user_threads < max_user_threads && pending_head) {
        admit_from_pending();
    }
}

int TCB::block_running() {
    TCB::running->state = BLOCKED;
    return WOULD_BLOCK;
}

void TCB::wake(TCB* t, uint64 retval) {
    if (!t) return;
    if (t->trap_frame) {
        uint64* frame = (uint64*)t->trap_frame;
        frame[FRAME_A0_IDX] = retval;
    }
    t->sem_result = (int)(long)retval;
    t->state = READY;
    Scheduler::put(t);
}

void TCB::init() {

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
    mainTCB->trap_frame   = nullptr;
    mainTCB->wait_n       = 0;
    mainTCB->sem_result   = 0;
    mainTCB->id           = 0;

    running = mainTCB;

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
    idleTCB->trap_frame   = nullptr;
    idleTCB->wait_n       = 0;
    idleTCB->sem_result   = 0;
    idleTCB->id           = 0;
    idleTCB->context[CTX_RA] = (uint64)&TCB::body_wrapper;
    idleTCB->context[CTX_SP] = (uint64)idle_stack_top;

    g_idle = idleTCB;

}
