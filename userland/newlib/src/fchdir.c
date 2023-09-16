#include <syscall.h>
#include <errno.h>

int fchdir(int fd)
{
	long result = syscall(SYS_fchdir, fd);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

