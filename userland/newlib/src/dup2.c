#include <syscall.h>
#include <errno.h>

int dup2(int oldfd, int newfd)
{
	long result = syscall(SYS_dup2, oldfd, newfd);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

