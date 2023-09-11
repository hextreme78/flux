#include <syscall.h>
#include <errno.h>

int execve(const char *pathname, char *const argv[], char *const envp[])
{
	long result = syscall(SYS_execve, pathname, argv, envp);
	errno = -result;
	return -1;
}

