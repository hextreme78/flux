#include <syscall.h>
#include <errno.h>
#undef errno
extern int errno;

int fchdir(int fd)
{
	long result = syscall(SYS_fchdir, fd);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

