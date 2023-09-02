#include <syscall.h>
#include <errno.h>
#undef errno
extern int errno;

int rmdir(const char *pathname)
{
	long result = syscall(SYS_rmdir, pathname);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

