---
name: reference-hwlib-symbols
description: hw.lib provided symbols we depend on — boot chain, MMIO addresses, PLIC helpers
metadata:
  type: reference
---

`lib/hw.lib` (78 symbols, 16 .o files) is the xv6-derived host runtime. Contents:

**Boot chain we enter through:**
- `_entry` (in `entry.o`) — QEMU jumps here at 0x80000000
- `start` (in `start.o`) — M-mode init, drops to S-mode via `mret`
- `system_main` (in `main.o`) — S-mode: `consoleinit`, `plicinit`, `uartinit`, `trapinit`, then calls **`main`** (ours)

**Constants exported (see `lib/hw.h`):**
- `HEAP_START_ADDR`, `HEAP_END_ADDR` — 128 MB heap region
- `CONSOLE_STATUS`, `CONSOLE_TX_DATA`, `CONSOLE_RX_DATA` — MMIO addresses (UART registers)
- `CONSOLE_IRQ = 10`, `CONSOLE_TX_STATUS_BIT = 1<<5`, `CONSOLE_RX_STATUS_BIT = 1`
- `DEFAULT_STACK_SIZE = 4096`, `DEFAULT_TIME_SLICE = 2`, `MEM_BLOCK_SIZE = 64`

**Callable helpers we may use:**
- `plic_claim() -> int` — returns IRQ number of pending external interrupt
- `plic_complete(int)` — ack the IRQ
- `__memset`, `__memmove` — string.o
- `panic`, `__printf`, `push_off`, `pop_off` — optional

**Traps:** hw.lib also exports `trapinit`/`kerneltrap`/`usertrap`, but we **override** `stvec` in `main()` after `system_main` returns, so those are vestigial.

**Entry-point contract:** `main.o` calls a symbol named `main` (no args, no return). Provide with `extern "C" void main()`.

**QEMU shutdown:** write `0x5555` (uint32) to physical address `0x100000` — that's the QEMU virt "test device" register.
