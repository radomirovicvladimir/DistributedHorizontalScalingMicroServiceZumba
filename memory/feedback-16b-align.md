---
name: feedback-16b-align
description: Node header is exactly 16 bytes so allocation payload is 16-byte SP-aligned as RISC-V requires
metadata:
  type: feedback
---

`MemoryAllocator::Node` = `{Node* next; size_t blocks;}` = 8 + 8 = **16 bytes** exactly. There is a `static_assert(sizeof(Node) == 16, ...)` guarding this.

**Why:** RISC-V requires `sp` to be 16-byte aligned at all times (PDF §"Osnovne karakteristike arhitekture", p.12). Thread stacks (Task 2) will use the payload pointer returned by the allocator as the top of the stack — payload MUST be 16-aligned. Because block starts are 64-byte-aligned (allocator rounds heap up to `MEM_BLOCK_SIZE`), and the header sits immediately before the payload, `payload_addr = block_start + 16` is 64-aligned + 16 = 16-aligned. If someone adds a field to `Node` and makes it 24 bytes, payloads land at `...18` — not 16-aligned — and `sd` on the stack faults.

**Why:** The static_assert catches this at compile time instead of at 3am with QEMU crashing on the first thread switch.

**How to apply:** Never widen `Node` without keeping its size a multiple of 16. If you need more metadata (e.g. a magic canary for double-free detection), reserve 16-byte chunks. Same rule applies to any header we place immediately before a returned payload pointer.
