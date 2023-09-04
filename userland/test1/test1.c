#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>

int main(void)
{
	char str[128];
	int fd;

	bzero(str, sizeof(str));

	mkdir("/dir0", 0777);
	mkdir("/dir0/dir1", 0777);

	chdir("dir0/dir1/");
	getcwd(str, sizeof(str));

	fd = open("/file0", O_RDWR | O_CREAT | O_TRUNC, 0777);
	write(fd, str, strlen(str));
	close(fd);

	return 0;
}

