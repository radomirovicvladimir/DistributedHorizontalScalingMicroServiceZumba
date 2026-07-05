#pragma once

#include "../lib/hw.h"

typedef struct _thread* thread_t;
typedef struct _sem*    sem_t;

#ifdef __cplusplus
extern "C" {
#endif

void*  mem_alloc(size_t size);
int    mem_free(void* ptr);

int    thread_create(thread_t* handle, void(*start_routine)(void*), void* arg);
int    thread_exit();
void   thread_dispatch();
int    getThreadId();
int    setMaximumThreads(int n);

int    sem_open(sem_t* handle, unsigned init);
int    sem_close(sem_t handle);
int    sem_wait(sem_t id);
int    sem_signal(sem_t id);
int    sem_wait_n(sem_t id, unsigned n);
int    sem_signal_n(sem_t id, unsigned n);

int    time_sleep(time_t);

#ifndef EOF
#define EOF (-1)
#endif
char   getc();
void   putc(char);

#ifdef __cplusplus
}
#endif
