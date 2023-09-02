#include <syscall.h>
#include <sys/types.h>
#include <errno.h>
#undef errno
extern int errno;

off_t _lseek(int fd, off_t offset, int whence)
{
	long result = syscall(SYS_lseek, fd, offset, whence);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

