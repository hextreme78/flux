#include <syscall.h>
#include <sys/types.h>
#include <errno.h>

int creat(const char *pathname, mode_t mode)
{
	long result = syscall(SYS_creat, pathname, mode);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

