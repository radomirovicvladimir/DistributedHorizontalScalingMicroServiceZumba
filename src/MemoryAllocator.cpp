#include "../h/MemoryAllocator.hpp"
#include "../h/debug.hpp"

MemoryAllocator::Header* MemoryAllocator::freelist = nullptr;

// ---- helpers --------------------------------------------------------------

static inline uint64 align_up(uint64 v, uint64 a)   { return (v + a - 1) & ~(a - 1); }
static inline uint64 align_down(uint64 v, uint64 a) { return v & ~(a - 1); }

// ---- init -----------------------------------------------------------------

void MemoryAllocator::init() {
    uint64 start = align_up  ((uint64)HEAP_START_ADDR, MEM_BLOCK_SIZE);
    uint64 end   = align_down((uint64)HEAP_END_ADDR,   MEM_BLOCK_SIZE);

    if (end <= start || (end - start) < 2 * MEM_BLOCK_SIZE) {
        kpanic("MemoryAllocator::init: heap region too small");
    }

    freelist = (Header*)start;
    freelist->next   = nullptr;
    freelist->blocks = (end - start) / MEM_BLOCK_SIZE;
}

// ---- alloc ----------------------------------------------------------------

void* MemoryAllocator::alloc(size_t bytes) {
    if (bytes == 0) return nullptr;

    // We need enough space for the user's payload AND our header, rounded up
    // to whole blocks.
    size_t need_bytes = bytes + sizeof(Header);
    // Overflow guard (degenerate but cheap).
    if (need_bytes < bytes) return nullptr;
    size_t need = (need_bytes + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;

    Header** pp = &freelist;
    for (Header* p = freelist; p != nullptr; pp = &p->next, p = p->next) {
        if (p->blocks < need) continue;

        if (p->blocks == need) {
            // Exact fit: unlink the whole node.
            *pp = p->next;
            return (void*)(p + 1);
        }

        // Split: shrink the front node in place, carve the tail off as the
        // allocation. Keeping the front linked means we don't have to fix *pp.
        p->blocks -= need;
        Header* tail = (Header*)((uchar*)p + p->blocks * MEM_BLOCK_SIZE);
        tail->blocks = need;
        return (void*)(tail + 1);
    }

    return nullptr;   // heap exhausted (or too fragmented for this request)
}

// ---- free -----------------------------------------------------------------

int MemoryAllocator::free(void* ptr) {
    if (ptr == nullptr) return 0;   // free(NULL) is a no-op

    Header* h = (Header*)ptr - 1;

    // Reject obviously-not-ours pointers.
    if ((uint64)h <  (uint64)HEAP_START_ADDR) return -1;
    if ((uint64)h >= (uint64)HEAP_END_ADDR)   return -1;
    if (h->blocks == 0)                       return -1;   // corrupt header

    // Find the freelist slot: largest `prev` with prev < h, and `cur` = its successor.
    Header *prev = nullptr, *cur = freelist;
    while (cur != nullptr && cur < h) { prev = cur; cur = cur->next; }

    // 1) Coalesce with successor if h and cur are adjacent in memory.
    uchar* h_end = (uchar*)h + h->blocks * MEM_BLOCK_SIZE;
    if (cur != nullptr && h_end == (uchar*)cur) {
        h->blocks += cur->blocks;
        h->next    = cur->next;
    } else {
        h->next = cur;
    }

    // 2) Coalesce with predecessor, or just splice in.
    if (prev != nullptr) {
        uchar* prev_end = (uchar*)prev + prev->blocks * MEM_BLOCK_SIZE;
        if (prev_end == (uchar*)h) {
            prev->blocks += h->blocks;
            prev->next    = h->next;
        } else {
            prev->next = h;
        }
    } else {
        freelist = h;
    }
    return 0;
}

// ---- check ----------------------------------------------------------------

void MemoryAllocator::check() {
    Header* prev = nullptr;
    for (Header* p = freelist; p != nullptr; prev = p, p = p->next) {
        if ((uint64)p < (uint64)HEAP_START_ADDR ||
            (uint64)p >= (uint64)HEAP_END_ADDR)
            kpanic("freelist: node out of bounds");
        if (p->blocks == 0)
            kpanic("freelist: zero-size node");
        if (prev != nullptr) {
            if (prev >= p)
                kpanic("freelist: not sorted");
            uchar* prev_end = (uchar*)prev + prev->blocks * MEM_BLOCK_SIZE;
            if (prev_end > (uchar*)p)
                kpanic("freelist: overlapping nodes");
            if (prev_end == (uchar*)p)
                kpanic("freelist: adjacent nodes not coalesced");
        }
    }
}

// ---- stats ----------------------------------------------------------------

size_t MemoryAllocator::total_free_bytes() {
    size_t total = 0;
    for (Header* p = freelist; p != nullptr; p = p->next)
        total += p->blocks * MEM_BLOCK_SIZE;
    return total;
}
