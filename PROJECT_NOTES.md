# OS1v2 — Complete Project Notes

Consolidated knowledge for the OS1 project (Univerzitet u Beogradu, ETF — RISC-V64 kernel inside QEMU). This file replaces all prior individual `.md` files. Source-of-truth disassembly transcripts live in `.claude-notes/lib-extracted/` and the Python disassembler is `.claude-notes/disasm.py`.

---

## 1. Project at a Glance

You build a small RISC-V multithreaded kernel on top of a stripped-down xv6 host. The host (in `lib/hw.lib`) handles boot, CPU init, UART hardware, timer hardware, and PLIC. Your kernel + the user app are statically linked into one binary and run inside QEMU.

**Project decisions taken:**
- **Non-preemptive kernel** (interrupts masked while in kernel — no spinlocks needed, simpler reasoning).
- **Per-thread kernel stack** (each TCB has its own kernel-mode stack; kernel code runs as nested calls in the current thread's context until `yield`).

**Boot chain (the symbol your code must provide is `main` or `system_main` depending on what `hw.lib/main.o` actually calls — verify before naming your entry):**

```
QEMU loads image at 0x80000000
        │
        ▼
_entry           ← hw.lib/entry.o      (sets sp, jumps to start)
        │
        ▼
start()          ← hw.lib/start.o      (M-mode setup, mret → S-mode)
        │
        ▼
system_main()    ← hw.lib/main.o       (S-mode: PLIC, UART, etc.)
        │
        ▼
main()           ← YOU PROVIDE         (your kernel + userMain thread)
```

Note: prior project notes disagreed about whether you provide `main` or `system_main`. `hw.lib/main.o` exports `system_main` (size 216 B) — so `main()` is the symbol *it* calls into, meaning **you provide `main()`**. (Confirm by checking `hw.lib/main.o` disassembly if anything fails to link.)

---

## 2. Interface Layering (from the PDF)

```
┌─────────────────────────────────────────┐
│  User App  (tests in app.lib, userMain) │
├─────────────────────────────────────────┤
│  C++ OO API   (Thread, Semaphore, …)    │  syscall_cpp.hpp   ← you write
│  C API        (thread_create, …)        │  syscall_c.hpp     ← you write
│  ABI          (ecall + arg packing)     │  asm wrappers      ← you write
├═════════════════════════════════════════┤  user/kernel boundary
│  Kernel (scheduler, alloc, semaphores)  │                    ← you write
├─────────────────────────────────────────┤
│  HW access module (hw.lib)              │  provided
└─────────────────────────────────────────┘
```

**Syscall flow** (e.g. `thread_create`):
1. User: `Thread t(...)` → C++ ctor → C API `thread_create()`
2. C API: allocate stack via `mem_alloc`, pack registers `a0=0x11, a1=handle, …`, execute `ecall`
3. CPU traps → jumps to `stvec` (your trap handler) → S-mode
4. Trap handler: read `scause`, dispatch on `a0`, run kernel's `Thread::create()`
5. `sret` → user mode resumes

---

## 3. What's Inside Each `.lib`

The `.lib` files are System V `ar` archives of RISC-V ELF64 object files. Use `.claude-notes/disasm.py` to re-extract symbols + disassembly without a toolchain.

### `hw.lib` — 78 symbols across 16 objects (complete xv6 host)

| Object | Provides |
|---|---|
| `entry.o`   | `_entry` (boot) |
| `start.o`   | `start`, `timerinit`, `stack0`, `timer_scratch` |
| `main.o`    | `system_main` (calls *your* `main`) |
| `console.o` | `consoleinit`, `consolewrite`, `consoleread`, `consoleintr`, `consputc`, `cons`, `devsw` |
| `uart.o`    | `uartinit`, `uartputc`, `uartputc_sync`, `uartgetc`, `uartintr`, `uartstart`, internal tx ring buf |
| `printf.o`  | `__printf`, `panic`, `printfinit`, `panicked` |
| `kalloc.o`  | `kalloc`, `kfree`, `kinit`, `freerange`, `kmem` — **page-grained**, xv6-internal use |
| `spinlock.o`| `initlock`, `acquire`, `release`, `holding`, `push_off`, `pop_off`, `push_on`, `pop_on` |
| `string.o`  | `__memset`, `__memmove` |
| `vm.o`      | `kvmincrease` (heap growth helper used by `__mem_alloc`) |
| `proc.o`    | `cpuid`, `mycpu`, `cpus`, `either_copyin/out`, `userinit` |
| `trap.o`    | `trapinit`, `trapinithart`, `kerneltrap`, `usertrap`, `usertrapret`, `clockintr`, `devintr`, `ticks`, `tickslock` — **vestigial; we override `stvec`** |
| `kernelvec.o`| xv6's trap-vector glue (not used) |
| `plic.o`    | **`plic_claim`**, **`plic_complete`**, `plicinit`, `plicinithart` |
| `support_cpp.o` | `_Unwind_Resume`, `__gxx_personality_v0` (stubs so linker is happy without exceptions) |
| `hw.o`      | Constants: `CONSOLE_STATUS`, `CONSOLE_TX_DATA`, `CONSOLE_RX_DATA`, `HEAP_START_ADDR`, `HEAP_END_ADDR` |

Symbols you'll actually call from your kernel: `plic_claim`, `plic_complete`, `__memset`, `__memmove`, the `CONSOLE_*`/`HEAP_*` constants, optionally `__printf`/`panic` for debug, `push_off`/`pop_off` for interrupt-mask nesting.

### `mem.lib` — 1 object (`mem.o`), exports `__mem_alloc` + `__mem_free`

- **K&R `malloc`** (Kernighan & Ritchie §8.7). 16-byte block header `{Header* ptr; uint32 size;}`. Circular freelist sorted by address, first-fit, coalesces adjacent free blocks on free.
- `__mem_alloc(size_t bytes)` — rounds to whole 16-byte units (`(nbytes+15)/16 + 1` — the +1 is the header). If freelist starves, calls **`kvmincrease`** (from `hw.lib/vm.o`) to extend the heap inside `HEAP_START_ADDR..HEAP_END_ADDR`. Returns `void*` or NULL.
- `__mem_free(void*)` — splices block into freelist, coalesces. Returns 0 on success.
- Internal state: `freep` (8 B `.sbss`) and `base` sentinel (16 B `.bss`), zero-initialized.
- **Not thread-safe**; rely on interrupt masking inside the kernel.

**Block-size mismatch trap:** the ABI passes `mem_alloc` size in **blocks** (`MEM_BLOCK_SIZE = 64`). `__mem_alloc` takes **bytes**. Your C-API wrapper must do `bytes = blocks * MEM_BLOCK_SIZE`.

### `console.lib` — 1 object (`putget.o`), exports `__putc`, `__getc`, `console_handler`

- `__putc(c)` → calls `console_write(/*user=*/0, &ch, /*n=*/1)` (xv6 cooked write).
- `__getc()` → calls `console_read(/*user=*/0, &ch, /*n=*/1)`. **Blocks on xv6's wait channel, not yours** — if you adopt this, console input "suspends the thread" via xv6 internals, not via your scheduler. Returns the char.
- `console_handler()` — reads `scause`, **panics if `sstatus.SIE` is set**, returns early if it's not an external IRQ (`scause == 0x8000000000000009`), then `plic_claim()` → if IRQ==10 calls `uartintr()` and `plic_complete()`; otherwise `__printf("unexpected interrupt irq=%d\n", …)` and completes anyway. **Does not touch timer IRQs.**
- **Not thread-safe**; safe to call only with interrupts masked.

**Wiring `console_handler` into your trap vector** (use this if you adopt `console.lib` instead of writing your own UART code):
```c
void c_trap_handler() {
    uint64 cause; asm("csrr %0, scause" : "=r"(cause));
    if (cause == (1ULL<<63 | 9))      console_handler();   // external IRQ
    else if (cause == (1ULL<<63 | 1)) /* timer: your code */;
    else if (cause == 8 || cause == 9)/* ecall: dispatch by a0 */;
}
```
PLIC is already initialized by `system_main()`, so you only need to set `sie.SEIE` (bit 9) before entering user mode.

---

## 4. Required Symbols You Must Provide

### Headers (in `h/`)

**`h/syscall_c.hpp`** — C API + opaque handle typedefs:
```c
typedef struct TCB* thread_t;
typedef struct SCB* sem_t;

int  thread_create(thread_t*, void(*)(void*), void*);
int  thread_exit();
void thread_dispatch();

int  sem_open(sem_t*, unsigned init);
int  sem_close(sem_t);
int  sem_wait(sem_t);
int  sem_signal(sem_t);
int  sem_wait_n(sem_t, unsigned n);
int  sem_signal_n(sem_t, unsigned n);

int  time_sleep(time_t);

void putc(char);
char getc();   // returns EOF (-1) on error

void* mem_alloc(size_t);
int   mem_free(void*);
```

**`h/syscall_cpp.hpp`** — OO façade. **Per PDF: must not add non-static data members, base classes, or change vtable layout/order. Existing function signatures must not change.** Skeleton (verbatim from PDF):
```cpp
void* ::operator new (size_t);
void  ::operator delete (void*);

class Thread {
public:
    Thread(void (*body)(void*), void* arg);
    virtual ~Thread();
    int start();
    static void dispatch();
    static int sleep(time_t);
protected:
    Thread();
    virtual void run() {}
private:
    thread_t myHandle;
    void (*body)(void*); void* arg;
};

class Semaphore {
public:
    Semaphore(unsigned init = 1);
    virtual ~Semaphore();
    int wait();
    int signal();
private:
    sem_t myHandle;
};

class PeriodicThread : public Thread {
public:
    void terminate();
protected:
    PeriodicThread(time_t period);
    virtual void periodicActivation() {}
private:
    time_t period;
};

class Console {
public:
    static char getc();
    static void putc(char);
};
```
- `new`/`delete` must wrap `mem_alloc`/`mem_free`.
- If `Thread(body, arg)` was used, `run()` must be **ignored even if overridden** in a derived class — function pointer wins.

**`h/riscv.h`** — CSR helpers:
```c
#define READ_CSR(csr)  ({ uint64 v; asm volatile("csrr %0, " #csr : "=r"(v)); v; })
#define WRITE_CSR(csr,val) asm volatile("csrw " #csr ", %0" :: "r"(val))

#define SSTATUS_SIE (1L << 1)
#define SSTATUS_SPP (1L << 8)
#define SSTATUS_SPIE (1L << 5)
#define SIE_SSIE    (1L << 1)
#define SIE_SEIE    (1L << 9)
```

### ABI — system call numbers (from PDF §"Interfejs jezgra")

| Code | C API | Notes |
|---|---|---|
| 0x01 | `mem_alloc(size)` | ABI receives `size` in **blocks** (your C wrapper converts) |
| 0x02 | `mem_free(ptr)`   | |
| 0x11 | `thread_create(handle, start, arg, stack_top)` | ABI takes the pre-allocated stack pointer — your C wrapper does the `mem_alloc` for the stack |
| 0x12 | `thread_exit()` | |
| 0x13 | `thread_dispatch()` | |
| 0x21 | `sem_open(handle, init)` | |
| 0x22 | `sem_close(handle)` | |
| 0x23 | `sem_wait(id)` | |
| 0x24 | `sem_signal(id)` | |
| 0x25 | `sem_wait_n(id, n)` | |
| 0x26 | `sem_signal_n(id, n)` | |
| 0x31 | `time_sleep(t)` | |
| 0x41 | `getc()` → char, `EOF=-1` on error | |
| 0x42 | `putc(char)` | |

**Register convention:** `a0` = syscall number on entry / return value on exit; `a1, a2, …` = args left-to-right per the C signature.

### Sources (in `src/`)

| File | Role |
|---|---|
| `src/main.cpp` | Your `main()` — init scheduler, set up `stvec`, create `userMain` thread, enter idle loop |
| `src/TCB.hpp/cpp` | Thread Control Block — stack ptr, saved context, body+arg, status, link ptrs |
| `src/Scheduler.hpp/cpp` | Ready queue (FIFO is enough for full points) |
| `src/SCB.hpp/cpp` | Semaphore Control Block — value, blocked-queue |
| `src/trap.S` | Trap vector entry — save regs, call C dispatcher, restore, `sret` |
| `src/trap.cpp` | C-side trap dispatcher — switch on `scause`, dispatch syscall by `a0` |
| `src/context.S` | `yield(oldCtx*, newCtx*)` — save callee-saved (ra, sp, s0-s11) into old, load from new, ret |
| `src/syscall.cpp` | C-API wrappers that issue `ecall`, plus the in-kernel implementations |
| `src/Thread.cpp` | C++ class implementations |
| `src/console.cpp` | (Task 4) tx/rx buffers + UART poll loop + ISR producer/consumer logic |

---

## 5. Implementation Plan (use the PDF's order)

The PDF's §"Predlog redosleda izrade" gives the canonical order — distilled:

| # | Step | Outcome |
|---|---|---|
| 0 | **Hello, RISC-V** | Single `main.cpp` that writes "Hello\n" by polling `CONSOLE_TX_DATA` and halts QEMU with `*(uint32*)0x100000 = 0x5555`. Verifies toolchain + `hw.lib` linkage. |
| 1 | `MemoryAllocator` | Wrap `__mem_alloc`/`__mem_free` from `mem.lib` first to unblock everything (write your own later for Task 1 points). |
| 2 | Trap skeleton | Install `stvec`, save regs, `sret` immediately. Trigger with `ecall` from `main`. |
| 3 | Syscall dispatch | Switch on `a0`, wire `mem_alloc`/`mem_free` through ABI + C API. Test from C program (no threads). |
| 4 | `Thread` + `Scheduler` skeleton | Define TCB, ready queue ops. |
| 5 | Context switch in asm | `yield(oldCtx, newCtx)` saving callee-saved set. |
| 6 | `thread_create` + `thread_dispatch` | Build initial context. Test with infinite-loop threads (kill QEMU manually). |
| 7 | `thread_exit` | Clean termination; kernel exits when last user thread done. |
| 8 | Semaphores | `sem_open/close/wait/signal/wait_n/signal_n`, blocked queue per sem. |
| 9 | Timer + time-sharing | Tick handler, quantum decrement, preempt on expiry. |
| 10 | `time_sleep` | Sleep list ordered by relative wake-time deltas. |
| 11 | Console out (`putc`) | TX buffer + kernel thread polls UART. |
| 12 | Console in (`getc`) | RX buffer + console ISR producer. |
| 13 | C++ API wrappers | Thread/Semaphore/Console/PeriodicThread. |

### Step 0 — Hello, RISC-V (verify the pipeline)

`src/main.cpp`:
```cpp
#include "../lib/hw.h"

extern "C" void main() {
    const char* msg = "Hello, RISC-V!\n";
    volatile char* tx = (volatile char*)CONSOLE_TX_DATA;
    volatile char* st = (volatile char*)CONSOLE_STATUS;
    for (const char* p = msg; *p; ++p) {
        while (!(*st & CONSOLE_TX_STATUS_BIT)) {}   // wait until tx-ready
        *tx = *p;
    }
    *(volatile unsigned int*)0x100000 = 0x5555;     // halt QEMU
    for (;;) {}
}
```
Run `make qemu`. If "Hello, RISC-V!" appears and QEMU exits, the toolchain + linkage works.

---

## 6. Trap Handling & Context Switch

### The trap vector (per-thread kernel stack model)

A single trap handler in `stvec` for syscalls + exceptions + external IRQs. **`sstatus.SIE` is cleared on trap entry by hardware** — you stay interrupt-masked through the entire kernel path (non-preemptive). Steps:

1. Switch sp to **this thread's kernel stack** (push_off / load kernel sp from TCB).
2. Save all caller-saved + any used callee-saved registers on that stack.
3. Read `scause` (preserve it before clobber).
4. Branch:
   - `scause == 8` or `9` (ecall from U/S) → syscall dispatch by `a0`.
   - `scause == (1ULL<<63) | 1` → timer tick: decrement quantum, fire sleeping-thread wakeups, possibly `yield`.
   - `scause == (1ULL<<63) | 9` → external IRQ: drain UART RX into buffer, or wake the TX-thread.
5. Restore registers, advance `sepc` past the `ecall` if needed, `sret`.

### `yield(oldCtx*, newCtx*)` — context switch primitive

Save callee-saved set into `oldCtx[]`, load from `newCtx[]`, `ret`. 14×8=112 B per context.

```asm
.global yield
yield:
    sd ra,  0*8(a0)
    sd sp,  1*8(a0)
    sd s0,  2*8(a0)
    sd s1,  3*8(a0)
    sd s2,  4*8(a0)
    sd s3,  5*8(a0)
    sd s4,  6*8(a0)
    sd s5,  7*8(a0)
    sd s6,  8*8(a0)
    sd s7,  9*8(a0)
    sd s8, 10*8(a0)
    sd s9, 11*8(a0)
    sd s10,12*8(a0)
    sd s11,13*8(a0)

    ld ra,  0*8(a1)
    ld sp,  1*8(a1)
    ld s0,  2*8(a1)
    ld s1,  3*8(a1)
    ld s2,  4*8(a1)
    ld s3,  5*8(a1)
    ld s4,  6*8(a1)
    ld s5,  7*8(a1)
    ld s6,  8*8(a1)
    ld s7,  9*8(a1)
    ld s8, 10*8(a1)
    ld s9, 11*8(a1)
    ld s10,12*8(a1)
    ld s11,13*8(a1)
    ret
```

Why only callee-saved? Caller code already saved its caller-saved set across the `yield` call site — that's the ABI guarantee. **However**, for asynchronous preemption on timer IRQ, the trap path must save the **full** register set (all 31 GPRs), because the preempted user code may have live temporaries in `t0..t6`/`a0..a7`. So you need two paths: sync `yield` saves 14 regs, async preemption (trap path) saves 31.

### TCB initial context

For a new thread that starts at `body(arg)`:
1. Allocate stack (`DEFAULT_STACK_SIZE` = 4096 bytes — declared in `hw.h`).
2. `sp = stack_top` (16-byte aligned, top of allocated block; stack grows down).
3. `context[CTX_RA] = (uint64)thread_wrapper` where `thread_wrapper` is a static function that calls `body(arg)` then `thread_exit()`.
4. Stash `body`+`arg` in the TCB so `thread_wrapper` can pick them up.

```cpp
extern "C" void thread_wrapper() {
    TCB* self = TCB::running;
    self->body(self->arg);
    thread_exit();   // never returns
}
```

---

## 7. Constants Already in `lib/hw.h`

```c
typedef uint64 size_t;
typedef uint64 time_t;

static const size_t DEFAULT_STACK_SIZE = 4096;
static const size_t DEFAULT_TIME_SLICE = 2;
static const size_t MEM_BLOCK_SIZE = 64;

extern const void* HEAP_START_ADDR;
extern const void* HEAP_END_ADDR;
extern const uint64 CONSOLE_STATUS;
extern const uint64 CONSOLE_TX_DATA;
extern const uint64 CONSOLE_RX_DATA;

static const uint64 CONSOLE_IRQ           = 10;
static const uint64 CONSOLE_TX_STATUS_BIT = 1 << 5;
static const uint64 CONSOLE_RX_STATUS_BIT = 1;

int  plic_claim(void);
void plic_complete(int irq);
```

`HEAP_START_ADDR`/`HEAP_END_ADDR` are set up by `hw.lib`; you don't initialize them. `MEM_BLOCK_SIZE` is fixed at 64 (within the PDF's 64-1024 allowed range).

---

## 8. Build, Run, Debug

Project root contains `Makefile` and `kernel.ld`. The Makefile auto-discovers `src/**/*.{cpp,c,S}` and links with `lib/{hw,mem,console}.lib`. Compiler flags worth knowing:
- `-nostdlib -ffreestanding -fno-common` — no libc, no host runtime.
- `-march=rv64ima -mabi=lp64 -mcmodel=medany -mno-relax` — RV64I + M + A.
- C++ is `-std=c++11 -fno-rtti -fno-threadsafe-statics` — no exceptions, no thread-local guards.
- Output binary: `kernel`; `kernel.asm` is a generated objdump for inspection.

```bash
make            # build
make qemu       # run inside qemu-system-riscv64 (CPU_CORE_COUNT=1)
make qemu-gdb   # build + start QEMU paused, waiting for gdb on port (id%5000+25000)
make clean      # wipe build/ and outputs
```

GDB session (from another terminal):
```
gdb-multiarch
(gdb) target remote localhost:<port from Makefile>
(gdb) symbol-file kernel
(gdb) c
(gdb) info reg
(gdb) si
```

To skip `mem.lib` or `console.lib` (i.e. write everything yourself for full Task 1 / Task 4 points), remove the corresponding entry from `LIBS = …` in the Makefile.

---

## 9. Grading (PDF §"Način ocenjivanja")

| Task | Title | Points |
|---|---|---|
| 1 | Memory allocation (`mem_alloc`/`mem_free`) | 5 |
| 2 | Threads (`thread_create/exit/dispatch`) + sync context switch | 10 |
| 3 | Semaphores | 5 |
| 4 | Async + time-sharing + `time_sleep`+`getc`/`putc`+`PeriodicThread` | 10 |
| 5 | Bonus: defended in pre-rok | 10 |

Each task must implement **all three layers** (ABI + C API + C++ API). Tasks 3/4/5 require Task 2. Skipping Task 1 → link `mem.lib`. Skipping Task 4 → link `console.lib` and call `console_handler()` from your trap vector. Pass conditions: at least 20 pts worth of tasks passing public tests, plus at least 15 pts at defense (private tests + modifications + viva).

### Submission

ZIP with two folders only:
- `src/` — all `.cpp` and `.S`
- `inc/` — all `.h`/`.hpp`

No binaries, no libs, no tests, no git repo. Upload at `http://rti.etf.bg.ac.rs/domaci/index.php?servis=os1_projekat` by the deadline (first business day before the exam date).

---

## 10. Common Pitfalls

- **Stack grows down on RISC-V.** Allocate at the lowest address, point `sp` at the top, and keep `sp` 16-byte aligned (hardware requires this).
- **Async trap saves all 31 GPRs**, not just callee-saved. Sync `yield` saves 14.
- **Always have something runnable.** Have an idle thread (`wfi` loop) so the scheduler never returns `nullptr`.
- **Don't trust `sp` is yours.** On trap entry, you may still be on the user thread's kernel stack — switch to the thread's kernel stack early (or use `sscratch` to stash it).
- **C++ globals/`new`.** Don't use ugraded `new`/`delete` — override them to call your `mem_alloc`/`mem_free`. Don't use libstdc++.
- **Block vs byte for `mem_alloc`.** PDF makes the ABI take blocks; the K&R lib takes bytes; the conversion lives in your C-API wrapper.
- **`console.lib`'s `__getc` blocks via xv6's wait channel**, not your scheduler — fine for early bring-up, problematic if you need cooperative scheduling around I/O.
- **`console_handler` panics if `sstatus.SIE` is set on entry.** Hardware clears it on trap entry; just don't re-enable interrupts before calling it.
- **Don't link both your `console.cpp` + `console.lib`** — duplicate symbols.
- **C++ exception/RTTI stubs.** If linker complains about `_Unwind_Resume`/`__gxx_personality_v0`, they're provided by `hw.lib/support_cpp.o`. Don't define them yourself.

---

## 11. Disassembler Tooling

`.claude-notes/disasm.py` is a pure-Python RV64IMA ELF inspector — sections, symbol table, `.rodata`/`.data`/`.bss` dumps, and `.text` disassembled with reloc annotations. Re-run after any new `.lib` extraction; it ignores everything that isn't an ELF64. Use it whenever you need to *prove* what a library actually does instead of trusting prose.

Already-generated transcripts:
- `.claude-notes/lib-extracted/mem/_disasm.txt`
- `.claude-notes/lib-extracted/console/_disasm.txt`

---

## 12. References

- PDF: `Projektni zadatak 2026 v1.0.pdf` — the canonical spec.
- xv6-riscv source: <https://github.com/mit-pdos/xv6-riscv> (everything in `hw.lib` is derived from this).
- RISC-V ISA manual: <https://riscv.org/technical/specifications/>
- OOP course materials (linked from the PDF): <http://oop.etf.rs>
