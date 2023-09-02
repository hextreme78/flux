#include <stddef.h>
#include "syscall.h"

int main(void)
{
	char str[] = "Hello, world!\n";
	int fd;

	creat("testfile", 0777);

	link("testfile", "testlink");

	fd = open("/testlink", O_RDWR);

	write(fd, str, sizeof(str));

	close(fd);

	debug_printint(123);

	return 0;
}

