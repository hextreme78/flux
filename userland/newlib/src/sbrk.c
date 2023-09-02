#include <syscall.h>
#include <stdint.h>
#include <errno.h>
#undef errno
extern int errno;

void *_sbrk(intptr_t increment)
{
	long result = syscall(SYS_sbrk, increment);
	if (result < 0) {
		errno = -result;
		return (void *) -1;
	}
	return (void *) result;
}

