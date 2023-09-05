#include <syscall.h>
#include <errno.h>
#undef errno
extern int errno;

int dup(int fd)
{
	long result = syscall(SYS_dup, fd);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

