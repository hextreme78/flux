#include <syscall.h>
#include <sys/types.h>
#include <errno.h>

ssize_t read(int fd, void *buf, size_t count)
{
	long result = syscall(SYS_read, fd, buf, count);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

