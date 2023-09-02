#include <syscall.h>

void exit(int status)
{
	syscall(SYS_exit, status);
	for (;;);
}

