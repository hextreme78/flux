#include <syscall.h>
#include <sys/types.h>
#include <errno.h>
#undef errno
extern int errno;

int _kill(pid_t pid, int sig)
{
	long result = syscall(SYS_kill, pid, sig);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

