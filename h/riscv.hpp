#pragma once
#include "../lib/hw.h"

#define READ_CSR(csr) ({ \
    uint64 _v; \
    asm volatile("csrr %0, " #csr : "=r"(_v)); \
    _v; \
})

#define WRITE_CSR(csr, val) do { \
    uint64 _v = (val); \
    asm volatile("csrw " #csr ", %0" :: "r"(_v)); \
} while (0)

#define SSTATUS_SIE  (1UL << 1)
#define SSTATUS_SPIE (1UL << 5)
#define SSTATUS_SPP  (1UL << 8)

#define SIE_SSIE     (1UL << 1)
#define SIE_SEIE     (1UL << 9)

#define SCAUSE_INT_BIT     (1UL << 63)
#define SCAUSE_ECALL_U     8
#define SCAUSE_ECALL_S     9

#define SCAUSE_S_SOFT_INT  (SCAUSE_INT_BIT | 1)
#define SCAUSE_S_EXT_INT   (SCAUSE_INT_BIT | 9)
