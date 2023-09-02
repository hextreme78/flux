#include <syscall.h>
#include <errno.h>
#undef errno
extern int errno;

int isatty(int fd)
{
	long result = syscall(SYS_isatty, fd);
	if (result < 0) {
		errno = -result;
		return 0;
	}
	return 1;
}

