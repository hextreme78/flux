#include <fcntl.h>
#include <unistd.h>

int main(void)
{
	char str[] = "Hello, world!\n";
	int fd;

	fd = open("/testfile", O_RDWR | O_CREAT);
	write(fd, str, sizeof(str));
	close(fd);

	return 0;
}

