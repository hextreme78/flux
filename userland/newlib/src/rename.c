#include <syscall.h>
#include <errno.h>
#undef errno
extern int errno;

int rename(const char *oldpath, const char *newpath)
{
	long result = syscall(SYS_rename, oldpath, newpath);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}
