#include <syscall.h>
#include <sys/types.h>
#include <errno.h>
#undef errno
extern int errno;

struct stat;

int _fstat(int fd, struct stat *statbuf)
{
	long result = syscall(SYS_fstat, fd, statbuf);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

