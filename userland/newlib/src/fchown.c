#include <syscall.h>
#include <sys/types.h>
#include <errno.h>

int fchown(int fd, uid_t uid, gid_t gid)
{
	long result = syscall(SYS_fchown, fd, uid, gid);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

