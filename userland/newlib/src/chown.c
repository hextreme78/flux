#include <syscall.h>
#include <sys/types.h>
#include <errno.h>

int chown(const char *path, uid_t uid, gid_t gid)
{
	long result = syscall(SYS_chown, path, uid, gid);
	if (result < 0) {
		errno = -result;
		return -1;
	}
	return result;
}

