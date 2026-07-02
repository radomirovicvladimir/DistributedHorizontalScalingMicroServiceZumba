---
name: feedback-alloc-two-entry
description: MemoryAllocator exposes both alloc(bytes) and alloc_blocks(payload) — kernel uses the block-based back door to avoid re-entering ecall
metadata:
  type: feedback
---

`MemoryAllocator::alloc_blocks(size_t payload)` and `MemoryAllocator::alloc(size_t bytes)` coexist deliberately.

**Why:** The user-facing C API `mem_alloc(bytes)` traps into the kernel with a block count, and the trap dispatcher forwards the count straight into `alloc_blocks` — zero unit conversion in the kernel. Kernel-internal code (Task 2 stack allocation, Task 3 SCB allocation, future TCB pools) also needs to allocate but **must not use `new`** (see [[feedback-kernel-no-new]]), so it calls `MemoryAllocator::alloc_blocks` directly. The `alloc(bytes)` variant is a thin `((bytes + 63) / 64)` rounding wrapper used by tests.

**Why:** Every conversion is a bug surface (off-by-one, rounding direction, block-size drift). Localising unit conversion to exactly one place (the C-API `mem_alloc` in `src/syscall.cpp`) means kernel-side code never touches bytes.

**How to apply:** For any new kernel-internal allocation in Tasks 2/3, compute the size in blocks and call `alloc_blocks`. E.g. for a TCB:
```cpp
size_t blocks = (sizeof(TCB) + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
void* raw = MemoryAllocator::alloc_blocks(blocks);
TCB* t = new(raw) TCB(...);
```
Never call `new TCB(...)` from kernel code — it would re-issue `SYS_MEM_ALLOC` through ecall and recurse into the trap handler.
