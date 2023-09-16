#include <syscall.h>
#include <sys/stat.h>
#include <errno.h>

int fstat(int fd, struct stat *statbuf)
{
	long result = syscall(SYS_fstat, fd, statbuf);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

