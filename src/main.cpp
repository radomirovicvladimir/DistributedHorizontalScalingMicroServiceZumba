#include "../lib/hw.h"
#include "../h/debug.hpp"
#include "../h/riscv.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/syscall_c.h"

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

// ---- direct-call tests (no trap) -----------------------------------------

static void direct_tests() {
    kputs("\n-- direct (MemoryAllocator::*) --\n");
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

    // Free everything first; first-fit then hands back the same address.
    // (Freeing one while others are live can't guarantee reuse — the big
    // free remainder usually appears first in the address-sorted list.)
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

// ---- e2e tests (through ecall) -------------------------------------------

static void e2e_tests() {
    kputs("\n-- e2e (mem_alloc / mem_free via ecall) --\n");
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

// ---- stress / fragmentation ----------------------------------------------

static void stress_tests() {
    kputs("\n-- stress --\n");
    size_t free0 = MemoryAllocator::free_bytes();

    const int N = 64;
    void* buf[N]; int got = 0;
    while (got < N && (buf[got] = mem_alloc(4096))) got++;
    kputs("  allocated 4KB blocks: "); kputdec(got); kputc('\n');
    CHECK("64 x 4KB fit", got == N);

    bool desc = true;
    for (int i = 1; i < got; i++)
        if (buf[i] >= buf[i-1]) { desc = false; break; }
    CHECK("4KB allocs descend", desc);

    for (int i = 0; i < got; i++) mem_free(buf[i]);
    MemoryAllocator::check();
    CHECK("stress free restored", MemoryAllocator::free_bytes() == free0);

    const int M = 40;
    void* sm[M];
    for (int i = 0; i < M; i++) sm[i] = mem_alloc(128);
    for (int i = 0; i < M; i += 2) mem_free(sm[i]);
    MemoryAllocator::check();
    for (int i = 1; i < M; i += 2) mem_free(sm[i]);
    MemoryAllocator::check();
    CHECK("fragmentation restored", MemoryAllocator::free_bytes() == free0);
}

// ---- entry ----------------------------------------------------------------

extern "C" void main() {
    kputs("==== OS1 boot ====\n");
    kputs("HEAP "); kputhex((uint64)HEAP_START_ADDR);
    kputs(" .. ");  kputhex((uint64)HEAP_END_ADDR);
    kputs(" ("); kputdec((uint64)HEAP_END_ADDR - (uint64)HEAP_START_ADDR);
    kputs(" B)\n");

    MemoryAllocator::init();
    direct_tests();

    WRITE_CSR(stvec, (uint64)&trap_entry);
    e2e_tests();
    stress_tests();

    kputs("\n==== Task 1: ");
    kputdec(n_run - n_fail); kputc('/'); kputdec(n_run);
    kputs(" passed ====\n");

    if (n_fail) kpanic("one or more tests failed");
    khalt();
}
