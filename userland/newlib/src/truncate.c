#include <syscall.h>
#include <sys/types.h>
#include <errno.h>

int truncate(const char *path, off_t length)
{
	long result = syscall(SYS_truncate, path, length);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

