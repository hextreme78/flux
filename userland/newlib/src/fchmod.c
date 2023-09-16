#include <syscall.h>
#include <sys/types.h>
#include <errno.h>

int fchmod(int fd, mode_t mode)
{
	long result = syscall(SYS_fchmod, fd, mode);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

