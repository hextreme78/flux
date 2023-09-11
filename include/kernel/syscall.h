#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#define SYS_exit      0
#define SYS_close     1
#define SYS_execve    2
#define SYS_fork      3
#define SYS_stat      4
#define SYS_getpid    5
#define SYS_isatty    6
#define SYS_kill      7
#define SYS_link      8
#define SYS_lseek     9
#define SYS_open      10
#define SYS_read      11
#define SYS_sbrk      12
#define SYS_times     13
#define SYS_unlink    14
#define SYS_wait      15
#define SYS_write     16

#define SYS_chdir     17
#define SYS_mkdir     18
#define SYS_readlink  19
#define SYS_rename    20
#define SYS_rmdir     21
#define SYS_symlink   22
#define SYS_ftruncate 23
#define SYS_mknod     24
#define SYS_getcwd    25
#define SYS_fcntl     26
#define SYS_dup       27
#define SYS_dup2      28
#define SYS_dup3      29
#define SYS_lstat     30
#define SYS_umask     31
#define SYS_chmod     32
#define SYS_chown     33
#define SYS_lchown    34
#define SYS_readdir   35
#define SYS_mkfifo    36
#define SYS_access    37
#define SYS_utimes    38
#define SYS_sync      39
#define SYS_fsync     40
#define SYS_fdatasync 41
#define SYS_pipe2     42

void syscall(void);

#endif

