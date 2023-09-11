#include <syscall.h>
#include <errno.h>

int chdir(const char *path)
{
	long result = syscall(SYS_chdir, path);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

