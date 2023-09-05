#include <syscall.h>
#include <sys/types.h>
#include <errno.h>
#undef errno
extern int errno;

struct stat;

int lchown(const char *pathname, uid_t uid, gid_t gid)
{
	long result = syscall(SYS_lchown, pathname, uid, gid);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

