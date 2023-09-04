#include <unistd.h>
#include <fcntl.h>

int ftruncate(int fd, off_t length);

int truncate(const char *path, off_t length)
{
	int fd = open(path, O_WRONLY);
	if (fd < 0) {
		return -1;
	}
	if (ftruncate(fd, length) < 0) {
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

