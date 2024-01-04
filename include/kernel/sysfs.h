#ifndef KERNEL_SYSFS_H
#define KERNEL_SYSFS_H

#include <kernel/fs.h>

ssize_t sys_read(int fd, void *buf, size_t count);
ssize_t sys_write(int fd, const void *buf, size_t count);
off_t sys_lseek(int fd, off_t offset, int whence);
int sys_close(int fd);
int sys_faccessat(int dirfd, const char *pathname, int mode, int flags);
int sys_dup(int oldfd);
int sys_dup2(int oldfd, int newfd);
int sys_fstatat(int fd, const char *path, struct stat *st, int flags);
ssize_t sys_readlink(const char *restrict path, char *restrict buf,
		size_t bufsize);
int sys_truncate(int fd, const char *path, off_t length, int flags);
int sys_unlinkat(int dirfd, const char *pathname, int flags);
int sys_openat(int dirfd, const char *pathname, int flags, mode_t mode);
int sys_getcwd(char *buf, size_t size);
int sys_chdir(int fd, const char *path, int flags);
int sys_linkat(int olddirfd, const char *oldpath,
		int newdirfd, const char *newpath, int flags);
int sys_renameat2(int olddirfd, const char *oldpath,
		int newdirfd, const char *newpath, unsigned int flags);
int sys_fcntl(int fd, int cmd, int arg);
int sys_fchmodat(int dirfd, const char *pathname, mode_t mode, int flags);
int sys_pipe2(int *pipefd, int flags);
int sys_mknodat(int dirfd, const char *path, mode_t mode, dev_t dev, const char *symlink);
mode_t sys_umask(mode_t mask);
int sys_fchownat(int dirfd, const char *pathname,
		uid_t owner, gid_t group, int flags);

#endif

