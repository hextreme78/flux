#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

int fchmod(int fd, mode_t mode);

int chmod(const char *path, mode_t mode)
{
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -1;
	}
	if (fchmod(fd, mode) < 0) {
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

