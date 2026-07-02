---
name: reference-console-lib
description: console.lib exports and how to wire console_handler / __putc / __getc into a Task-4-skipping build
metadata:
  type: reference
---

`lib/console.lib` (one `.o` — `putget.o`) is the Task-4 fallback. Three exported symbols:

**`void __putc(char c)`** — calls xv6's `console_write(user=0, &c, n=1)`. Blocking until the character clears xv6's cooked-write path. Not thread-safe; call only with interrupts masked.

**`char __getc(void)`** — calls `console_read(user=0, &c, n=1)`. **Blocks on xv6's wait channel, NOT on our scheduler** — so a thread doing `getc` on an empty input will be suspended by xv6 machinery, not by our `Thread::sleep`/semaphore infrastructure. Fine for our simple 20-point build.

**`void console_handler(void)`** — the console ISR:
1. Panics if `sstatus.SIE == 1` on entry (must be called with interrupts masked; hardware clears SIE on trap entry so this is fine).
2. Returns early unless `scause == (1<<63)|9` (S-mode external interrupt).
3. Calls `plic_claim()`. If IRQ == 10 → calls `uartintr()` and `plic_complete(10)`. Else → `__printf` "unexpected interrupt" and completes anyway.
4. Does NOT touch timer interrupts.

**Wiring into our trap dispatcher (in `src/trap.cpp` c_trap_handler):**
```c
if (cause == (SCAUSE_INT_BIT | 9)) {          // external IRQ
    console_handler();
    return;                                    // don't advance sepc; async
}
if (cause == 8 || cause == 9) {                // ecall
    switch (f->a0) { … case SYS_PUTC: __putc(f->a1); break; … }
    WRITE_CSR(sepc, pc + 4);
}
```

**Sie bit** — must enable `sie.SEIE` (bit 9) before entering the user's first thread, else the PLIC IRQs never get delivered to us. (PLIC itself is already initialized by `system_main`.)

**Duplicate-symbol trap:** do NOT ship both a hand-written `console.cpp` and link `console.lib` — you'll get a link error on `__putc`/`__getc`.
