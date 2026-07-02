---
name: reference-abi-syscalls
description: ABI syscall numbers from PDF §"Interfejs jezgra" and quirks per call
metadata:
  type: reference
---

Register convention: `a0` = syscall code / return value. `a1..a4` = args left-to-right per C signature.

| Code | C signature | ABI quirk |
|---|---|---|
| 0x01 | `void* mem_alloc(size_t bytes)` | ABI receives size in **blocks** (`MEM_BLOCK_SIZE = 64`); C-API wrapper converts |
| 0x02 | `int mem_free(void*)` | 0 ok, negative = err |
| 0x11 | `int thread_create(thread_t*, void(*)(void*), void*)` | ABI takes a **4th arg**: `stack_space` — pre-allocated by C-API |
| 0x12 | `int thread_exit()` | terminates caller |
| 0x13 | `void thread_dispatch()` | may switch to another ready thread |
| 0x21 | `int sem_open(sem_t*, unsigned init)` | |
| 0x22 | `int sem_close(sem_t)` | wakes all waiters with error |
| 0x23 | `int sem_wait(sem_t)` | |
| 0x24 | `int sem_signal(sem_t)` | |
| 0x25 | `int sem_wait_n(sem_t, unsigned n)` | atomic n-unit wait |
| 0x26 | `int sem_signal_n(sem_t, unsigned n)` | atomic n-unit signal |
| 0x31 | `int time_sleep(time_t)` | sleeps in timer-period units |
| 0x41 | `char getc()` | returns `EOF=-1` on error |
| 0x42 | `void putc(char)` | |

**Two ABI extensions vs C API:**
1. `mem_alloc` — C-API converts bytes→blocks. Rounding: `(bytes + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE`.
2. `thread_create` — C-API allocates the stack (`mem_alloc(DEFAULT_STACK_SIZE)`) and passes the pointer as `a4`. The ABI receives 4 args: `handle`, `start_routine`, `arg`, `stack_top`.

Sync trap: `scause == 8` (ecall from U) or `9` (ecall from S). Async: `scause == (1<<63)|1` = timer soft-int, `scause == (1<<63)|9` = external IRQ (console). Trap PC lives in `sepc`; advance by 4 after `ecall`.
