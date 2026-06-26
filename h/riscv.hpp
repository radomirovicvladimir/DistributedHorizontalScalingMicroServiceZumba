#pragma once
#include "../lib/hw.h"

// RISC-V S-mode CSR access helpers and bit definitions.
// Per PDF §"Obrada sistemskih poziva...". Only the bits/CSRs we actually use.

#define READ_CSR(csr) ({ \
    uint64 _v; \
    asm volatile("csrr %0, " #csr : "=r"(_v)); \
    _v; \
})

#define WRITE_CSR(csr, val) do { \
    uint64 _v = (val); \
    asm volatile("csrw " #csr ", %0" :: "r"(_v)); \
} while (0)

// sstatus bits
#define SSTATUS_SIE  (1UL << 1)   // S-mode interrupt enable
#define SSTATUS_SPIE (1UL << 5)   // previous SIE (saved on trap)
#define SSTATUS_SPP  (1UL << 8)   // previous privilege (0=U, 1=S)

// sie bits
#define SIE_SSIE     (1UL << 1)   // software interrupt enable
#define SIE_SEIE     (1UL << 9)   // external interrupt enable

// scause values (low bits; bit 63 = "is async interrupt")
#define SCAUSE_INT_BIT     (1UL << 63)
#define SCAUSE_ECALL_U     8
#define SCAUSE_ECALL_S     9
// async ones used later:
#define SCAUSE_S_SOFT_INT  (SCAUSE_INT_BIT | 1)   // timer (delivered as soft int per PDF)
#define SCAUSE_S_EXT_INT   (SCAUSE_INT_BIT | 9)   // PLIC external (console)
