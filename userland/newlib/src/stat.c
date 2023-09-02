#include <syscall.h>
#include <sys/types.h>
#include <errno.h>
#undef errno
extern int errno;

struct stat;

int _stat(const char *restrict pathname, struct stat *restrict statbuf)
{
	long result = syscall(SYS_stat, pathname, statbuf);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

