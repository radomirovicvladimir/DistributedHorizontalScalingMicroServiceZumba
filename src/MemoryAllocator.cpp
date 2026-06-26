#include "../h/MemoryAllocator.hpp"
#include "../h/debug.hpp"

MemoryAllocator::Node* MemoryAllocator::head = nullptr;

static inline uint64  up  (uint64 v, uint64 a) { return (v + a - 1) & ~(a - 1); }
static inline uint64  down(uint64 v, uint64 a) { return v & ~(a - 1); }
static inline uchar*  end_of(MemoryAllocator::Node* n) {
    return (uchar*)n + n->blocks * MEM_BLOCK_SIZE;
}

void MemoryAllocator::init() {
    uint64 s = up  ((uint64)HEAP_START_ADDR, MEM_BLOCK_SIZE);
    uint64 e = down((uint64)HEAP_END_ADDR,   MEM_BLOCK_SIZE);
    if (e < s + 2 * MEM_BLOCK_SIZE) kpanic("heap too small");
    head = (Node*)s;
    head->next   = nullptr;
    head->blocks = (e - s) / MEM_BLOCK_SIZE;
}

void* MemoryAllocator::alloc_blocks(size_t payload) {
    if (payload == 0) return nullptr;
    size_t need = payload + 1;                    // +1 for our header block
    if (need < payload) return nullptr;           // overflow

    for (Node **pp = &head, *p = head; p; pp = &p->next, p = p->next) {
        if (p->blocks < need) continue;
        if (p->blocks <= need + 1) {               // take whole chunk
            *pp = p->next;
            return p + 1;
        }
        p->blocks -= need;                         // split off tail
        Node* tail = (Node*)((uchar*)p + p->blocks * MEM_BLOCK_SIZE);
        tail->blocks = need;
        return tail + 1;
    }
    return nullptr;
}

void* MemoryAllocator::alloc(size_t bytes) {
    if (bytes == 0) return nullptr;
    return alloc_blocks((bytes + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE);
}

int MemoryAllocator::free(void* ptr) {
    if (!ptr) return 0;
    Node* h = (Node*)ptr - 1;
    if ((uint64)h < (uint64)HEAP_START_ADDR ||
        (uint64)h >= (uint64)HEAP_END_ADDR ||
        h->blocks == 0) return -1;

    Node *prev = nullptr, *cur = head;
    while (cur && cur < h) { prev = cur; cur = cur->next; }
    // Double-free: h is already in (or inside) a free node.
    if (cur == h || (prev && (uchar*)h < end_of(prev))) return -1;

    h->next = cur;
    if (cur && end_of(h) == (uchar*)cur) {        // coalesce successor
        h->blocks += cur->blocks;
        h->next    = cur->next;
    }
    if (prev && end_of(prev) == (uchar*)h) {       // coalesce predecessor
        prev->blocks += h->blocks;
        prev->next    = h->next;
    } else if (prev) {
        prev->next = h;
    } else {
        head = h;
    }
    return 0;
}

void MemoryAllocator::check() {
    Node* prev = nullptr;
    for (Node* p = head; p; prev = p, p = p->next) {
        if ((uint64)p < (uint64)HEAP_START_ADDR ||
            (uint64)p >= (uint64)HEAP_END_ADDR) kpanic("freelist OOB");
        if (p->blocks == 0)                      kpanic("freelist zero-size");
        if (!prev) continue;
        if (prev >= p)                           kpanic("freelist unsorted");
        if (end_of(prev) >  (uchar*)p)           kpanic("freelist overlap");
        if (end_of(prev) == (uchar*)p)           kpanic("freelist not coalesced");
    }
}

size_t MemoryAllocator::free_bytes() {
    size_t t = 0;
    for (Node* p = head; p; p = p->next) t += p->blocks * MEM_BLOCK_SIZE;
    return t;
}
