#include <syscall.h>
#include <errno.h>

int pipe2(int *pipefd, int flags)
{
	long result = syscall(SYS_pipe2, pipefd, flags);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

