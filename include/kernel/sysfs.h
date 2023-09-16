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
int sys_chdir(const char *path);
int sys_symlink(const char *path1, const char *path2);
int sys_stat(const char *path, struct stat *st);
int sys_ftruncate(int fd, off_t length);
int sys_getcwd(char *buf, size_t size);
int sys_fcntl(int fd, int cmd, int arg);
int sys_dup(int oldfd);
int sys_dup2(int oldfd, int newfd);
int sys_dup3(int oldfd, int newfd, int flags);
int sys_lstat(const char *path, struct stat *st);
mode_t sys_umask(mode_t mask);
int sys_chmod(const char *path, mode_t mode);
int sys_chown(const char *path, uid_t uid, gid_t gid);
int sys_lchown(const char *path, uid_t uid, gid_t gid);
int sys_fstat(int fd, struct stat *st);
int sys_fchdir(int fd);
int sys_fchmod(int fd, mode_t mode);
int sys_fchown(int fd, uid_t uid, gid_t gid);
int sys_truncate(const char *path, off_t length);
int sys_creat(const char *path, mode_t mode);
int sys_access(const char *path, int mode);

#endif

