#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int stat(const char *restrict pathname, struct stat *restrict statbuf)
{
	int fd = open(pathname, O_RDONLY);
	if (fd < 0) {
		return -1;
	}
	if (fstat(fd, statbuf) < 0) {
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

