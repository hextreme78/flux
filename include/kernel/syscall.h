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

/* filesystem syscalls */
#define SYS_CREAT    20
#define SYS_LINK     21
#define SYS_MKDIR    22
#define SYS_UNLINK   23
#define SYS_RMDIR    24
#define SYS_RENAME   25
#define SYS_READLINK 26
#define SYS_OPEN     27
#define SYS_READ     28
#define SYS_WRITE    29
#define SYS_LSEEK    30
#define SYS_CLOSE    31
#define SYS_STAT     32
#define SYS_CHDIR    33

void syscall(void);

#endif

