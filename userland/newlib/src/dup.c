#include <syscall.h>
#include <errno.h>

int dup(int fd)
{
	long result = syscall(SYS_dup, fd);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

