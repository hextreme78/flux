#ifndef SYSCALL_H
#define SYSCALL_H

#define SYS_exit     0
#define SYS_close    1
#define SYS_execve   2
#define SYS_fork     3
#define SYS_fstat    4
#define SYS_getpid   5
#define SYS_isatty   6
#define SYS_kill     7
#define SYS_link     8
#define SYS_lseek    9
#define SYS_open     10
#define SYS_read     11
#define SYS_sbrk     12
#define SYS_stat     13
#define SYS_times    14
#define SYS_unlink   15
#define SYS_wait     16
#define SYS_write    17

#define SYS_chdir    18
#define SYS_mkdir    19
#define SYS_readlink 20
#define SYS_rename   21
#define SYS_rmdir    22
#define SYS_symlink  23

long syscall(long number, ...);

#endif

