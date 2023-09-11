#include <syscall.h>
#include <errno.h>

int close(int fd)
{
	long result = syscall(SYS_close, fd);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

