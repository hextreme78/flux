#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

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

void syscall(void);

#endif

