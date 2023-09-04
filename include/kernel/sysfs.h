#ifndef KERNEL_SYSFS_H
#define KERNEL_SYSFS_H

#include <kernel/fs.h>

int sys_link(const char *path1, const char *path2);
int sys_mkdir(const char *path, mode_t mode);
int sys_unlink(const char *path);
int sys_rmdir(const char *path);
int sys_rename(const char *path1, const char *path2);
ssize_t sys_readlink(const char *restrict path, char *restrict buf,
		size_t bufsize);
int sys_open(const char *path, int flags, mode_t mode);
ssize_t sys_read(int fd, void *buf, size_t count);
ssize_t sys_write(int fd, const void *buf, size_t count);
off_t sys_lseek(int fd, off_t offset, int whence);
int sys_close(int fd);
int sys_fchdir(int fd);
int sys_symlink(const char *path1, const char *path2);
int sys_fstat(int fd, struct stat *st);
int sys_ftruncate(int fd, off_t length);
int sys_getcwd(char *buf, size_t size);

#endif

