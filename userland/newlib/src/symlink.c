#include <syscall.h>
#include <errno.h>
#undef errno
extern int errno;

int symlink(const char *target, const char *linkpath)
{
	long result = syscall(SYS_symlink, target, linkpath);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}
