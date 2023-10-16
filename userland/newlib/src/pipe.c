#include <syscall.h>
#include <errno.h>

int pipe2(int *pipefd, int flags);

int pipe(int *pipefd)
{
	return pipe2(pipefd, 0);
}

