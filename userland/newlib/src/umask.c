#include <syscall.h>
#include <sys/types.h>
#include <errno.h>
#undef errno
extern int errno;

mode_t umask(mode_t mode)
{
	long result = syscall(SYS_umask, mode);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

