#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/riscv.hpp"
#include "../h/debug.hpp"

TCB* TCB::running = nullptr;
TCB* TCB::mainTCB = nullptr;
TCB* TCB::idleTCB = nullptr;

TCB* g_idle = nullptr;

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

void TCB::idle_body(void*  ) {

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

    void* frame_sp = TCB::seed_initial_frame(stack_top,  true);

    t->context[CTX_RA] = (uint64)&trap_return_tail;
    t->context[CTX_SP] = (uint64)frame_sp;

    *handle_out = t;
    Scheduler::put(t);
    return 0;
}

void TCB::exit() {
    TCB::running->state = FINISHED;

    Scheduler::switch_to_next();

    kpanic("thread_exit returned");
    for (;;) {}
}

void TCB::dispatch() {

    if (Scheduler::empty()) return;

    Scheduler::switch_to_next();
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
    idleTCB->context[CTX_RA] = (uint64)&TCB::body_wrapper;
    idleTCB->context[CTX_SP] = (uint64)idle_stack_top;

    g_idle = idleTCB;

}
