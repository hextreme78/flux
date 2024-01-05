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
int sys_getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
int sys_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);
int sys_setuid(uid_t uid);
int sys_setgid(gid_t gid);
int sys_setresuid(uid_t ruid, uid_t euid, uid_t suid);
int sys_setresgid(gid_t rgid, gid_t egid, uid_t sgid);
pid_t sys_getsid(pid_t pid);
pid_t sys_setsid(void);
pid_t sys_getpgid(pid_t pid);
int sys_setpgid(pid_t pid, pid_t pgid);
pid_t sys_gettid(void);

#endif

