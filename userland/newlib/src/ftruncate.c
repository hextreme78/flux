#include <syscall.h>
#include <sys/types.h>
#include <errno.h>
#undef errno
extern int errno;

int ftruncate(int fd, off_t length)
{
	long result = syscall(SYS_ftruncate, fd, length);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

