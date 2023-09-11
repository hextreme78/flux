#include <syscall.h>
#include <sys/types.h>
#include <errno.h>
#undef errno
extern int errno;

int chmod(const char *path, mode_t mode)
{
	long result = syscall(SYS_chmod, path, mode);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

