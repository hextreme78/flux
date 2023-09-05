#include <syscall.h>
#include <errno.h>
#undef errno
extern int errno;

struct stat;

int lstat(const char *restrict pathname, struct stat *restrict st)
{
	long result = syscall(SYS_lstat, pathname, st);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

