#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

/* debug syscall */
#define SYS_DEBUG_PRINTINT 100

/* process syscalls */
#define SYS__EXIT  0
#define SYS_WAIT   1
#define SYS_KILL   2
#define SYS_GETPID 3
#define SYS_FORK   4
#define SYS_EXECVE 5
#define SYS_SBRK   6

long syscall(long number, ...);

void _exit(int status);

typedef int64_t pid_t;
pid_t getpid(void);

void debug_printint(int64_t num);

#endif

