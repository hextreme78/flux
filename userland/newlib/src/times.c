#include <syscall.h>
#include <sys/types.h>
#include <errno.h>
#undef errno
extern int errno;

struct tms;

clock_t times(struct tms *buf)
{
	long result = syscall(SYS_times, buf);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

