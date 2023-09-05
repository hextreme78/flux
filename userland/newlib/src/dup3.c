#include <syscall.h>
#include <errno.h>
#undef errno
extern int errno;

int dup3(int oldfd, int newfd, int flags)
{
	long result = syscall(SYS_dup3, oldfd, newfd, flags);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

