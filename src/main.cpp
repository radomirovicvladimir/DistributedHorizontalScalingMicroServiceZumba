#include "../lib/hw.h"
#include "../h/debug.hpp"
#include "../h/riscv.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/syscall_c.h"
#include "../h/TCB.hpp"
#include "../h/Scheduler.hpp"

extern "C" void trap_entry();   // defined in trap_entry.S

// ---- test harness --------------------------------------------------------

static int n_run = 0, n_fail = 0;

#define CHECK(name, expr) do {                              \
    bool _ok = (expr);                                       \
    n_run++; if (!_ok) n_fail++;                             \
    kputs(_ok ? "  [ OK ] " : "  [FAIL] "); kputs(name);     \
    kputc('\n');                                             \
} while (0)

static bool in_heap(void* p) {
    return p && (uint64)p >= (uint64)HEAP_START_ADDR
              && (uint64)p <  (uint64)HEAP_END_ADDR;
}

// ============================================================================
// Task 1 — Memory allocator (kept intact from before)
// ============================================================================

static void mem_direct_tests() {
    kputs("\n-- Task 1 direct (MemoryAllocator::*) --\n");
    size_t free0 = MemoryAllocator::free_bytes();

    void *p1 = MemoryAllocator::alloc(100),
         *p2 = MemoryAllocator::alloc(4096),
         *p3 = MemoryAllocator::alloc(1);
    CHECK("alloc(100) in heap",  in_heap(p1));
    CHECK("alloc(4096) in heap", in_heap(p2));
    CHECK("alloc(1) in heap",    in_heap(p3));
    CHECK("p1 16-aligned", ((uint64)p1 & 0xF) == 0);
    CHECK("p2 16-aligned", ((uint64)p2 & 0xF) == 0);
    CHECK("p3 16-aligned", ((uint64)p3 & 0xF) == 0);
    MemoryAllocator::check();

    MemoryAllocator::free(p1);
    MemoryAllocator::free(p2);
    MemoryAllocator::free(p3);
    MemoryAllocator::check();
    CHECK("full free restored heap", MemoryAllocator::free_bytes() == free0);

    void* p1b = MemoryAllocator::alloc(100);
    CHECK("reuse after full free", p1b == p1);
    MemoryAllocator::free(p1b);

    CHECK("free(NULL) == 0",   MemoryAllocator::free(nullptr) == 0);
    CHECK("free(bogus) == -1", MemoryAllocator::free((void*)0xdeadbeefUL) == -1);

    void* dfp = MemoryAllocator::alloc(64);
    CHECK("first free == 0",    MemoryAllocator::free(dfp) == 0);
    CHECK("double-free == -1",  MemoryAllocator::free(dfp) == -1);
    MemoryAllocator::check();

    CHECK("alloc(0) == NULL", MemoryAllocator::alloc(0) == nullptr);
}

static void mem_e2e_tests() {
    kputs("\n-- Task 1 e2e (mem_alloc / mem_free via ecall) --\n");
    size_t free0 = MemoryAllocator::free_bytes();

    void *p1 = mem_alloc(100), *p2 = mem_alloc(4096);
    CHECK("ecall mem_alloc(100)",  in_heap(p1));
    CHECK("ecall mem_alloc(4096)", in_heap(p2));
    CHECK("ecall mem_free(p1)",    mem_free(p1) == 0);
    mem_free(p2);
    CHECK("ecall full free",       MemoryAllocator::free_bytes() == free0);

    void* p1b = mem_alloc(100);
    CHECK("ecall reuse", p1b == p1);
    mem_free(p1b);

    struct Foo { uint64 x[8]; virtual ~Foo() {} };
    Foo* f = new Foo;
    CHECK("new Foo in heap",  in_heap(f));
    delete f;
    CHECK("delete restored",  MemoryAllocator::free_bytes() == free0);
}

// ============================================================================
// Task 2 — Threads
//
// Each test creates one or more threads, cooperatively yields via
// thread_dispatch, and waits for completion by draining the ready queue via
// its own yields. Since main() itself is a TCB (mainTCB, built in TCB::init),
// we just call thread_dispatch() in a loop from main and inspect flags/counters
// set by the worker threads.
// ============================================================================

// Shared state for the thread tests — plain globals in .bss.
static volatile int t_flag_a = 0;
static volatile int t_flag_b = 0;
static volatile int t_counter = 0;
static volatile int t_order[16];
static volatile int t_order_len = 0;

static void body_setflag_a(void*) { t_flag_a = 1; }
static void body_setflag_b(void*) { t_flag_b = 1; }

static void body_incr(void* arg) {
    // Each of these threads bumps the counter 5 times, yielding between
    // increments to exercise interleaving.
    int rounds = (int)(uint64)arg;
    for (int i = 0; i < rounds; i++) {
        t_counter++;
        thread_dispatch();
    }
}

static void body_record_order(void* arg) {
    int id = (int)(uint64)arg;
    if (t_order_len < 16) t_order[t_order_len++] = id;
}

// Drain the ready queue by repeatedly yielding until nothing but idle is left.
// Because idle is never in the queue, the queue emptying is the signal that
// all user threads we launched have finished.
static void drain_ready_queue() {
    // Yield up to a bounded number of times to avoid runaway loops if a test
    // launches a thread that never terminates.
    for (int i = 0; i < 10000; i++) {
        if (Scheduler::empty()) return;
        thread_dispatch();
    }
    kpanic("drain_ready_queue: threads still running after 10k yields");
}

static void thread_tests_basic() {
    kputs("\n-- Task 2 basic (create/dispatch/exit) --\n");

    // 1. Single thread — sets a flag, then implicitly exits by falling off body.
    t_flag_a = 0;
    thread_t h1;
    CHECK("thread_create(setflag_a) == 0",
          thread_create(&h1, body_setflag_a, nullptr) == 0);
    drain_ready_queue();
    CHECK("worker A ran",  t_flag_a == 1);

    // 2. Two threads in FIFO order.
    t_flag_a = 0; t_flag_b = 0;
    thread_t h2a, h2b;
    thread_create(&h2a, body_setflag_a, nullptr);
    thread_create(&h2b, body_setflag_b, nullptr);
    drain_ready_queue();
    CHECK("workers A and B both ran", t_flag_a == 1 && t_flag_b == 1);
}

static void thread_tests_fifo_order() {
    kputs("\n-- Task 2 FIFO ordering (8 threads) --\n");

    t_order_len = 0;
    for (int i = 0; i < 16; i++) t_order[i] = -1;

    thread_t h[8];
    for (int i = 0; i < 8; i++) {
        thread_create(&h[i], body_record_order, (void*)(uint64)i);
    }
    drain_ready_queue();

    bool ok = (t_order_len == 8);
    for (int i = 0; ok && i < 8; i++) if (t_order[i] != i) ok = false;
    CHECK("threads ran in creation (FIFO) order", ok);
}

static void thread_tests_interleave() {
    kputs("\n-- Task 2 interleaving via thread_dispatch --\n");
    t_counter = 0;

    thread_t h[4];
    for (int i = 0; i < 4; i++) {
        thread_create(&h[i], body_incr, (void*)(uint64)5);
    }
    drain_ready_queue();

    // 4 threads × 5 increments = 20.
    CHECK("counter == 4*5 after interleaving", t_counter == 20);
}

static void thread_tests_nested_create() {
    // A thread that itself creates another thread. Exercises re-entrance
    // of SYS_THREAD_CREATE while we're already running on a worker's stack.
    kputs("\n-- Task 2 nested creation --\n");

    struct Nested {
        static void inner(void*) {
            t_flag_b = 1;
        }
        static void outer(void*) {
            t_flag_a = 1;
            thread_t hi;
            thread_create(&hi, inner, nullptr);
        }
    };

    t_flag_a = 0; t_flag_b = 0;
    thread_t ho;
    thread_create(&ho, Nested::outer, nullptr);
    drain_ready_queue();
    CHECK("outer ran",         t_flag_a == 1);
    CHECK("inner (nested) ran", t_flag_b == 1);
}

static void thread_tests_explicit_exit() {
    // Worker that calls thread_exit() explicitly instead of returning.
    // If TCB::exit's context switch is broken, we hang or crash here.
    kputs("\n-- Task 2 explicit thread_exit --\n");
    t_flag_a = 0;

    struct E {
        static void body(void*) {
            t_flag_a = 1;
            thread_exit();
            // unreachable
            t_flag_a = 99;
        }
    };

    thread_t h;
    thread_create(&h, E::body, nullptr);
    drain_ready_queue();
    CHECK("explicit exit reached flag", t_flag_a == 1);
    CHECK("explicit exit unreachable code untouched", t_flag_a != 99);
}

// ---- entry ----------------------------------------------------------------

extern "C" void main() {
    kputs("==== OS1 boot ====\n");
    kputs("HEAP "); kputhex((uint64)HEAP_START_ADDR);
    kputs(" .. ");  kputhex((uint64)HEAP_END_ADDR);
    kputs(" ("); kputdec((uint64)HEAP_END_ADDR - (uint64)HEAP_START_ADDR);
    kputs(" B)\n");

    // Task 1 setup + tests.
    MemoryAllocator::init();
    mem_direct_tests();

    WRITE_CSR(stvec, (uint64)&trap_entry);
    mem_e2e_tests();

    // Task 2 setup: build mainTCB (so `running` isn't null) and idleTCB
    // (so Scheduler::switch_to_next always has SOMETHING to run).
    TCB::init();

    // Task 2 tests.
    thread_tests_basic();
    thread_tests_fifo_order();
    thread_tests_interleave();
    thread_tests_nested_create();
    thread_tests_explicit_exit();

    kputs("\n==== Task 1+2: ");
    kputdec(n_run - n_fail); kputc('/'); kputdec(n_run);
    kputs(" passed ====\n");

    if (n_fail) kpanic("one or more tests failed");
    khalt();
}
