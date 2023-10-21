#include <syscall.h>
#include <sys/types.h>
#include <errno.h>

int mkfifo(const char *pathname, mode_t mode)
{
	long result = syscall(SYS_mkfifo, pathname, mode);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

