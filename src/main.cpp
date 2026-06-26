#include "../lib/hw.h"
#include "../h/debug.hpp"
#include "../h/riscv.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/syscall_c.h"

// Trap entry point — defined in trap.S.
extern "C" void trap_entry();

// ---- test helpers ---------------------------------------------------------

static int tests_run = 0;
static int tests_failed = 0;

static void check_bool(const char* name, bool ok) {
    tests_run++;
    kputs(ok ? "  [ OK ] " : "  [FAIL] ");
    kputs(name);
    kputc('\n');
    if (!ok) tests_failed++;
}

static bool in_heap(void* p) {
    return p != nullptr &&
           (uint64)p >= (uint64)HEAP_START_ADDR &&
           (uint64)p <  (uint64)HEAP_END_ADDR;
}

// ---- direct-call tests (no trap) -----------------------------------------

static void direct_tests() {
    kputs("\n-- direct (MemoryAllocator::*) --\n");

    MemoryAllocator::check();
    size_t free0 = MemoryAllocator::total_free_bytes();

    void* p1 = MemoryAllocator::alloc(100);
    void* p2 = MemoryAllocator::alloc(4096);
    void* p3 = MemoryAllocator::alloc(1);
    check_bool("alloc(100) in heap",  in_heap(p1));
    check_bool("alloc(4096) in heap", in_heap(p2));
    check_bool("alloc(1) in heap",    in_heap(p3));
    check_bool("alloc 16-aligned p1", ((uint64)p1 & 0xF) == 0);
    check_bool("alloc 16-aligned p2", ((uint64)p2 & 0xF) == 0);
    check_bool("alloc 16-aligned p3", ((uint64)p3 & 0xF) == 0);

    MemoryAllocator::check();

    // Reuse: free p1, alloc same size — should land at same address.
    MemoryAllocator::free(p1);
    void* p1b = MemoryAllocator::alloc(100);
    check_bool("reuse after free", p1b == p1);

    // Coalesce: free p1b and p2 (adjacent in our split scheme they should be)
    // then alloc one big block.
    MemoryAllocator::free(p1b);
    MemoryAllocator::free(p3);
    MemoryAllocator::free(p2);
    MemoryAllocator::check();
    check_bool("free restored full heap", MemoryAllocator::total_free_bytes() == free0);

    // free(NULL) is no-op.
    check_bool("free(NULL) == 0", MemoryAllocator::free(nullptr) == 0);

    // Bogus free is rejected.
    check_bool("free(bogus) == -1",
               MemoryAllocator::free((void*)0xdeadbeefUL) == -1);

    // Zero alloc.
    check_bool("alloc(0) == NULL", MemoryAllocator::alloc(0) == nullptr);
}

// ---- e2e tests (through ecall) -------------------------------------------

static void e2e_tests() {
    kputs("\n-- e2e (mem_alloc / mem_free via ecall) --\n");

    size_t free0 = MemoryAllocator::total_free_bytes();

    void* p1 = mem_alloc(100);
    void* p2 = mem_alloc(4096);
    check_bool("ecall mem_alloc(100)",  in_heap(p1));
    check_bool("ecall mem_alloc(4096)", in_heap(p2));

    int r1 = mem_free(p1);
    check_bool("ecall mem_free(p1) == 0", r1 == 0);
    void* p1b = mem_alloc(100);
    check_bool("ecall reuse", p1b == p1);

    mem_free(p1b);
    mem_free(p2);
    check_bool("ecall full free restored heap",
               MemoryAllocator::total_free_bytes() == free0);

    // C++ new/delete (also goes through ecall).
    struct Foo { uint64 x[8]; virtual ~Foo() {} };
    Foo* f = new Foo;
    check_bool("new Foo in heap", in_heap(f));
    delete f;
    check_bool("delete restored heap",
               MemoryAllocator::total_free_bytes() == free0);
}

// ---- stress / fragmentation ----------------------------------------------

static void stress_tests() {
    kputs("\n-- stress --\n");

    size_t free0 = MemoryAllocator::total_free_bytes();

    // Exhaust the heap with 4 KB allocations, then free them all.
    const int N = 64;
    void* buf[N];
    int allocated = 0;
    for (int i = 0; i < N; i++) {
        buf[i] = mem_alloc(4096);
        if (!buf[i]) break;
        allocated++;
    }
    kputs("  allocated 4KB blocks: "); kputdec(allocated); kputc('\n');
    check_bool("at least 8 x 4KB fit", allocated >= 8);

    for (int i = 0; i < allocated; i++) mem_free(buf[i]);
    MemoryAllocator::check();
    check_bool("stress: free restored full heap",
               MemoryAllocator::total_free_bytes() == free0);

    // Fragmentation: alloc 40 small, free every other, then try one big.
    const int M = 40;
    void* small[M];
    for (int i = 0; i < M; i++) small[i] = mem_alloc(128);
    for (int i = 0; i < M; i += 2) mem_free(small[i]);
    MemoryAllocator::check();
    // After freeing odd-indexed-only, freelist has many small holes; clean up.
    for (int i = 1; i < M; i += 2) mem_free(small[i]);
    MemoryAllocator::check();
    check_bool("fragmentation: full free restored heap",
               MemoryAllocator::total_free_bytes() == free0);
}

// ---- entry ----------------------------------------------------------------

extern "C" void main() {
    kputs("==== OS1 boot ====\n");
    kputs("HEAP_START_ADDR = "); kputhex((uint64)HEAP_START_ADDR); kputc('\n');
    kputs("HEAP_END_ADDR   = "); kputhex((uint64)HEAP_END_ADDR);   kputc('\n');
    kputs("HEAP_SIZE       = ");
        kputdec((uint64)HEAP_END_ADDR - (uint64)HEAP_START_ADDR);  kputs(" bytes\n");

    MemoryAllocator::init();
    kputs("free after init = "); kputdec(MemoryAllocator::total_free_bytes());
    kputs(" bytes\n");

    direct_tests();

    // Install our trap vector and run the same tests through ecall.
    WRITE_CSR(stvec, (uint64)&trap_entry);
    kputs("\nstvec installed: "); kputhex((uint64)&trap_entry); kputc('\n');

    e2e_tests();
    stress_tests();

    kputs("\n==== Task 1 tests: ");
    kputdec(tests_run - tests_failed); kputc('/'); kputdec(tests_run);
    kputs(" passed ====\n");

    if (tests_failed != 0) panic("one or more tests failed");
    kputs("ALL OK — halting QEMU\n");
    khalt();
}
