#include <syscall.h>
#include <errno.h>
#undef errno
extern int errno;

int _unlink(const char *pathname)
{
	long result = syscall(SYS_unlink, pathname);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

