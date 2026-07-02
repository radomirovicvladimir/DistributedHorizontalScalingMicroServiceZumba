---
name: project-dir-layout
description: Project directory layout — sources, headers, libs, build outputs
metadata:
  type: project
---

```
DistributedHorizontalScalingMicroServiceZumba/
├─ Makefile              — builds kernel binary; auto-globs src/**/{cpp,c,S}
├─ kernel.ld             — linker script, ENTRY(_entry), image at 0x80000000
├─ kernel                — built ELF (gitignored)
├─ kernel.asm            — objdump for grepping
├─ build/                — .o / .d / .lst files
├─ lib/                  — provided libs (do NOT modify)
│   ├─ hw.lib   + hw.h        (boot, UART, PLIC, heap constants)
│   ├─ mem.lib  + mem.h       (unused — we wrote our own allocator)
│   └─ console.lib + console.h (used for Task 4 fallback: __putc/__getc/console_handler)
├─ h/                    — kernel headers
│   ├─ debug.hpp, riscv.hpp, syscall_abi.hpp, MemoryAllocator.hpp
│   ├─ syscall_c.h       — required by PDF for C API
│   └─ syscall_cpp.hpp   — required by PDF for C++ API (skeleton exists, needs impl in Tasks 2/3)
└─ src/                  — kernel sources
    ├─ debug.cpp, cpp_runtime.cpp
    ├─ MemoryAllocator.cpp
    ├─ trap_entry.S, trap.cpp
    ├─ syscall.cpp
    └─ main.cpp          — test harness / entry
```

**Makefile knob:** `LIBS = hw.lib console.lib` — no mem.lib (we replaced it). Kernel entry point per `hw.lib/main.o` is `main` (not `system_main`).

**Submission format** (per PDF): ZIP with two folders only, `src/` and `inc/`. Rename `h/` → `inc/` at zip time. No libs, no binaries, no tests, no git.

**Build/run:**
- `make` — build
- `make qemu` — run (`-nographic`, Ctrl-A X to force-exit)
- `make qemu-gdb` — start paused, listen for gdb on `id % 5000 + 25000`
- `make clean`

TOOLPREFIX auto-detects `riscv64-unknown-elf-` or `riscv64-linux-gnu-`. On Vlado's Windows box, likely the WSL/msys `riscv64-unknown-elf-` toolchain is what fires. Build must be run from a POSIX shell (Makefile uses `find … -printf`, which is a GNU find feature).
