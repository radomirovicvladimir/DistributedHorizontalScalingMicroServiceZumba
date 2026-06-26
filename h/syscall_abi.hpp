#pragma once

// ABI syscall numbers (PDF §"Interfejs jezgra", table p.6-8).
// Convention: a0 = syscall code on entry / return value on exit.
//             a1, a2, ... = args left-to-right per the C signature.

#define SYS_MEM_ALLOC       0x01    // a1 = size IN BLOCKS (not bytes)
#define SYS_MEM_FREE        0x02    // a1 = ptr

#define SYS_THREAD_CREATE   0x11    // a1=handle*, a2=start, a3=arg, a4=stack_top
#define SYS_THREAD_EXIT     0x12
#define SYS_THREAD_DISPATCH 0x13

#define SYS_SEM_OPEN        0x21
#define SYS_SEM_CLOSE       0x22
#define SYS_SEM_WAIT        0x23
#define SYS_SEM_SIGNAL      0x24
#define SYS_SEM_WAIT_N      0x25
#define SYS_SEM_SIGNAL_N    0x26

#define SYS_TIME_SLEEP      0x31

#define SYS_GETC            0x41
#define SYS_PUTC            0x42
