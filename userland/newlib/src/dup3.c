#include <syscall.h>
#include <errno.h>

int dup3(int oldfd, int newfd, int flags)
{
	long result = syscall(SYS_dup3, oldfd, newfd, flags);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

