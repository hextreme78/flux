#include <syscall.h>
#include <sys/types.h>
#include <errno.h>

pid_t wait(int *wstatus)
{
	long result = syscall(SYS_wait, wstatus);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

