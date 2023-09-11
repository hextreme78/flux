#include <syscall.h>
#include <sys/types.h>
#include <errno.h>

ssize_t readlink(const char *restrict pathname, char *restrict buf, size_t bufsiz)
{
	long result = syscall(SYS_readlink, pathname, buf, bufsiz);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

