#ifndef SYSCALL_H
#define SYSCALL_H

#define SYS_exit   93
#define SYS_close  57
#define SYS_execve 0
#define SYS_fork   0
#define SYS_fstat  80
#define SYS_getpid 172
#define SYS_isatty 0
#define SYS_kill   129
#define SYS_link   1025
#define SYS_lseek  62
#define SYS_open   1024
#define SYS_read   63
#define SYS_sbrk   0
#define SYS_stat   1038
#define SYS_times  153
#define SYS_unlink 1026
#define SYS_wait   0
#define SYS_write  64

long syscall(long number, ...);

#endif

