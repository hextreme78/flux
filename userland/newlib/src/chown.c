#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

int fchown(int fd, uid_t uid, gid_t gid);

int chown(const char *path, uid_t uid, gid_t gid)
{
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -1;
	}
	if (fchown(fd, uid, gid) < 0) {
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

