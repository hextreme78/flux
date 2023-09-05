#include <fcntl.h>
#include <unistd.h>

int creat(const char *pathname, mode_t mode)
{
	int fd = open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
	if (fd < 0) {
		return fd;
	}
	close(fd);
	return 0;
}

