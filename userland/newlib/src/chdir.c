#include <unistd.h>
#include <fcntl.h>

int fchdir(int fd);

int chdir(const char *path)
{
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -1;
	}
	if (fchdir(fd) < 0) {
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

