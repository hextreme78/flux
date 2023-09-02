#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

/* debug syscall */
#define SYS_DEBUG_PRINTINT 100

/* newlib syscalls */
#define SYS_exit   93
#define SYS_close  57
#define SYS_execve -1
#define SYS_fork   -2
#define SYS_fstat  80
#define SYS_getpid 172
#define SYS_isatty -3
#define SYS_kill   129
#define SYS_link   1025
#define SYS_lseek  62
#define SYS_open   1024
#define SYS_read   63
#define SYS_sbrk   -4
#define SYS_stat   1038
#define SYS_times  153
#define SYS_unlink 1026
#define SYS_wait   -5
#define SYS_write  64

#define SYS_chdir    18
#define SYS_creat    19
#define SYS_mkdir    20
#define SYS_readlink 21
#define SYS_rename   22
#define SYS_rmdir    23
#define SYS_symlink  24

void syscall(void);

#endif

