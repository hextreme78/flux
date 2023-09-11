#include <syscall.h>
#include <sys/stat.h>
#include <errno.h>
#undef errno
extern int errno;

int stat(const char *path, struct stat *statbuf)
{
	long result = syscall(SYS_stat, path, statbuf);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

