#pragma once
#include "../lib/hw.h"

class MemoryAllocator {
public:
    static void   init();
    static void*  alloc(size_t bytes);
    static void*  alloc_blocks(size_t payload);
    static int    free(void* ptr);
    static void   check();
    static size_t free_bytes();

private:
    struct Node { Node* next; size_t blocks; };
    static_assert(sizeof(Node) == 16, "Node must be 16 bytes");
    static Node* head;
    static uchar* end_of(Node* n) {
        return (uchar*)n + n->blocks * MEM_BLOCK_SIZE;
    }

    MemoryAllocator() = delete;
};
