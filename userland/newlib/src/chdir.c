#include <syscall.h>
#include <sys/types.h>
#include <errno.h>
#undef errno
extern int errno;

int chdir(const char *path)
{
	long result = syscall(SYS_chdir, path);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

