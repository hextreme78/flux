#include <syscall.h>
#include <sys/types.h>
#include <errno.h>
#undef errno
extern int errno;

pid_t _wait(int *wstatus)
{
	long result = syscall(SYS_wait, wstatus);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

