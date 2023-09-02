#include <syscall.h>
#include <sys/types.h>
#include <errno.h>
#undef errno
extern int errno;

ssize_t write(int fd, const void *buf, size_t count)
{
	long result = syscall(SYS_write, fd, buf, count);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

