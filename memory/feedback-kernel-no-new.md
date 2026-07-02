---
name: feedback-kernel-no-new
description: Kernel code must never use C++ `new` — it would recurse into the ecall path since our operator new wraps mem_alloc
metadata:
  type: feedback
---

Per PDF p.21: kernel code MUST NOT use `new` — the global `operator new` in `src/cpp_runtime.cpp` forwards to `mem_alloc` which issues an `ecall`. From the kernel side, that means:

1. Kernel calls `new TCB(...)`.
2. Compiler emits `mem_alloc(sizeof(TCB))` (the C-API wrapper).
3. `mem_alloc` executes `ecall`.
4. CPU traps into `trap_entry.S`, which calls `c_trap_handler`.
5. `c_trap_handler` calls `MemoryAllocator::alloc_blocks`.
6. Allocation succeeds, but we're now nested inside the trap handler while we were already in kernel context — kernel stack has doubled, `sepc`/`scause` may already be clobbered by the outer trap, and if any TCB constructor issues another syscall we recurse further.

Even if step 6 "works", it's a landmine: it breaks the "kernel is one atomic critical section" invariant, corrupts the trap-frame layout, and (once interrupts are enabled) races with itself.

**Why:** the whole point of `mem_alloc` as an ABI syscall is that it's a user→kernel boundary. Kernel code is on the other side of that boundary already.

**How to apply:** in Tasks 2/3 whenever you'd write `new TCB(...)` / `new SCB(...)`, do:
```cpp
void* raw = MemoryAllocator::alloc_blocks(blocks_needed);
if (!raw) return nullptr;
TCB* t = new (raw) TCB(constructor_args);   // placement new — pure ctor, no allocation
```
And for destruction:
```cpp
t->~TCB();                     // explicit dtor call, no delete
MemoryAllocator::free(t);
```
See [[feedback-alloc-two-entry]] for why `alloc_blocks` is the right entry point.
