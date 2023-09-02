#include <syscall.h>
#include <errno.h>
#undef errno
extern int errno;

int close(int fd)
{
	long result = syscall(SYS_close, fd);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

