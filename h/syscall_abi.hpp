#pragma once

#define SYS_MEM_ALLOC       0x01
#define SYS_MEM_FREE        0x02

#define SYS_THREAD_CREATE   0x11
#define SYS_THREAD_EXIT     0x12
#define SYS_THREAD_DISPATCH 0x13
#define SYS_THREAD_GET_ID   0x14
#define SYS_SET_MAX_THREADS 0x15

#define SYS_SEM_OPEN        0x21
#define SYS_SEM_CLOSE       0x22
#define SYS_SEM_WAIT        0x23
#define SYS_SEM_SIGNAL      0x24
#define SYS_SEM_WAIT_N      0x25
#define SYS_SEM_SIGNAL_N    0x26

#define SYS_TIME_SLEEP      0x31

#define SYS_GETC            0x41
#define SYS_PUTC            0x42
