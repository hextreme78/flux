#ifndef KERNEL_SYSPROC_H
#define KERNEL_SYSPROC_H

#include <kernel/proc.h>
#include <abi-bits/utsname.h>
#include <abi-bits/resource.h>

void sys_exit(int status);
pid_t sys_getpid(void);
pid_t sys_getppid(void);
int sys_sleep(const struct timespec *req, struct timespec *rem);
pid_t sys_fork(void);
int sys_execve(const char *pathname, char *const argv[], char *const envp[]);
int sys_uname(struct utsname *buf);
pid_t sys_wait4(pid_t pid, int *status, int options, struct rusage *rusage);
int sys_brk(void *addr);
uid_t sys_getuid(void);
uid_t sys_geteuid(void);
gid_t sys_getgid(void);
gid_t sys_getegid(void);

#endif

