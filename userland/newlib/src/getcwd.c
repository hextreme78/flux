#include <syscall.h>
#include <stddef.h>
#include <errno.h>

char *getcwd(char *buf, size_t size)
{
	long result = syscall(SYS_getcwd, buf, size);
	if (result < 0) {
		errno = -result;
		return NULL;
	}
	return buf;
}

