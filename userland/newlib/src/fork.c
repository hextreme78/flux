#include <syscall.h>
#include <sys/types.h>
#include <errno.h>
#undef errno
extern int errno;

pid_t fork(void)
{
	long result = syscall(SYS_fork);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

