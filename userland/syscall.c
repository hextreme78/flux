#include "syscall.h"
#include <stdarg.h>

void _exit(int status)
{
	syscall(SYS__EXIT, status);
	for (;;);
}

pid_t getpid(void)
{
	return syscall(SYS_GETPID);
}

void debug_printint(int64_t num)
{
	syscall(SYS_DEBUG_PRINTINT, num);
}

int open(const char *path, int flags, ...)
{
	va_list args;
	mode_t mode;
	
	va_start(args, flags);
	mode = va_arg(args, mode_t);
	va_end(args);

	return syscall(SYS_OPEN, path, flags, mode);
}

ssize_t read(int fd, void *buf, size_t count)
{
	return syscall(SYS_READ, fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count)
{
	return syscall(SYS_WRITE, fd, buf, count);
}

ssize_t lseek(int fd, off_t offset, int whence)
{
	return syscall(SYS_LSEEK, fd, offset, whence);
}

int close(int fd)
{
	return syscall(SYS_CLOSE, fd);
}

int creat(const char *path, mode_t flags)
{
	return syscall(SYS_CREAT, path, flags);
}

int link(const char *old, const char *new)
{
	return syscall(SYS_LINK, old, new);
}

int mkdir(const char *path, mode_t mode)
{
	return syscall(SYS_MKDIR, path, mode);
}

int unlink(const char *path)
{
	return syscall(SYS_UNLINK, path);
}

int rmdir(const char *path)
{
	return syscall(SYS_RMDIR, path);
}

int rename(const char *old, const char *new)
{
	return syscall(SYS_RENAME, old, new);
}

ssize_t readlink(const char *path, char *buf, size_t sz)
{
	return syscall(SYS_READLINK, path, buf, sz);
}

int stat(const char *path, struct stat *st)
{
	return syscall(SYS_STAT, path, st);
}

int chdir(const char *path)
{
	return syscall(SYS_CHDIR, path);
}

