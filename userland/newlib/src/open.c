#include <syscall.h>
#include <sys/types.h>
#include <errno.h>
#undef errno
extern int errno;

int open(const char *pathname, int flags, mode_t mode)
{
	long result = syscall(SYS_open, pathname, flags, mode);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

