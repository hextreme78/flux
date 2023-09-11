#include <syscall.h>
#include <errno.h>

int fcntl(int fd, int flags, int arg)
{
	long result = syscall(SYS_fcntl, fd, flags, arg);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

