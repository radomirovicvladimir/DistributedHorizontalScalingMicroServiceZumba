---
name: project-task1-done
description: Task 1 complete — MemoryAllocator + all three interface layers, 24 in-tree tests passing
metadata:
  type: project
---

Task 1 (5 pts) is fully implemented and tested. Files that own each responsibility:

| File | Layer |
|---|---|
| `src/debug.cpp`, `h/debug.hpp` | Polling UART helpers (`kputc`/`kpanic`/`khalt`) — safe before any traps |
| `h/riscv.hpp` | CSR read/write macros + sstatus/sie/scause bit defs |
| `h/syscall_abi.hpp` | Shared syscall codes (`SYS_MEM_ALLOC=0x01`, `SYS_MEM_FREE=0x02`, `SYS_THREAD_*`, `SYS_SEM_*`, `SYS_TIME_SLEEP=0x31`, `SYS_GETC=0x41`, `SYS_PUTC=0x42`) |
| `h/MemoryAllocator.hpp`, `src/MemoryAllocator.cpp` | Kernel-side allocator — first-fit, address-sorted coalescing freelist, `alloc(bytes)` + `alloc_blocks(payload)` + `free()` + `check()` + `free_bytes()` |
| `src/trap_entry.S` | ABI: saves 16 caller-saved regs, calls C dispatcher, restores, `sret` |
| `src/trap.cpp` | C-side switch on `f->a0`, currently only `SYS_MEM_ALLOC`/`SYS_MEM_FREE` — needs Task 2 cases added |
| `src/syscall.cpp` | C API: `mem_alloc(bytes)` (converts to blocks) and `mem_free`; ecall1 helper. Need to add ecall0/ecall4 for Task 2 |
| `src/cpp_runtime.cpp` | C++ API: `operator new/delete` (+ `[]` + sized) forwarding to `mem_alloc`/`mem_free` |
| `src/main.cpp` | Test harness: 13 direct + 7 e2e + 4 stress tests, prints "24/24 passed" |

**Node layout is 16 bytes** — `Node* next` + `size_t blocks`. Header immediately precedes payload, so payload starts 16-aligned given 64-byte-aligned block starts — see [[feedback-16b-align]].

**Two entry points on purpose:** `alloc(bytes)` for user path, `alloc_blocks(payload)` for kernel-internal callers (Task 2 stack allocation, Task 3 SCB allocation) and for the trap dispatcher (which gets block counts from the ABI). See [[feedback-alloc-two-entry]].

**Kernel must never use `new`** — see [[feedback-kernel-no-new]]. TCB/SCB allocation in Tasks 2/3 must use `MemoryAllocator::alloc_blocks(...)` + placement-new.
