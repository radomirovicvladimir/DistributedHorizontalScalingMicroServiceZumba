#include "../h/syscall_c.h"
#include "printing.hpp"
#include "MemAllocStress_test.hpp"

static bool inRange(void* p) {
    return p != 0
        && (uint64)p >= (uint64)HEAP_START_ADDR
        && (uint64)p <  (uint64)HEAP_END_ADDR;
}

void MemAllocStress_test() {
    printString("--- MemAllocStress ---\n");

    const int N = 200;
    void* ptrs[N];

    printString("Phase 1: sequential alloc(64B) x ");
    printInt(N);
    printString("\n");
    int got = 0;
    for (int i = 0; i < N; i++) {
        ptrs[i] = mem_alloc(64);
        if (!inRange(ptrs[i])) break;
        got++;
    }
    printString("  allocated: ");
    printInt(got);
    printString("\n");
    if (got != N) { printString("  FAIL: not all allocs succeeded\n"); return; }

    printString("Phase 2: alignment check\n");
    int misaligned = 0;
    for (int i = 0; i < got; i++) {
        if (((uint64)ptrs[i] & 0xF) != 0) misaligned++;
    }
    printString("  misaligned: ");
    printInt(misaligned);
    printString("\n");
    if (misaligned) { printString("  FAIL: some pointers not 16-aligned\n"); return; }

    printString("Phase 3: free every 2nd, then re-alloc\n");
    for (int i = 0; i < got; i += 2) {
        mem_free(ptrs[i]);
        ptrs[i] = 0;
    }
    int reused = 0;
    for (int i = 0; i < got; i += 2) {
        ptrs[i] = mem_alloc(64);
        if (inRange(ptrs[i])) reused++;
    }
    printString("  re-allocated: ");
    printInt(reused);
    printString("\n");

    printString("Phase 4: free all\n");
    for (int i = 0; i < got; i++) {
        if (ptrs[i]) mem_free(ptrs[i]);
    }

    printString("Phase 5: single big alloc should succeed (coalesce proof)\n");
    void* big = mem_alloc(64 * N);
    if (!inRange(big)) {
        printString("  FAIL: big alloc after freeing all small allocs\n");
        return;
    }
    mem_free(big);
    printString("  OK big-alloc after coalesce\n");

    printString("Phase 6: mem_alloc(0) must return NULL\n");
    void* z = mem_alloc(0);
    if (z != 0) { printString("  FAIL: mem_alloc(0) returned non-null\n"); return; }
    printString("  OK\n");

    printString("Phase 7: mem_free(NULL) must not crash\n");
    int r = mem_free(0);
    printString("  mem_free(NULL) returned ");
    printInt(r);
    printString(" (0 is expected)\n");

    printString("Phase 8: mem_free(bogus) must return non-zero, not crash\n");
    int rb = mem_free((void*)0xdeadbeef);
    printString("  mem_free(0xdeadbeef) returned ");
    printInt(rb);
    printString(" (non-zero expected)\n");

    printString("MemAllocStress: PASS\n");
}
