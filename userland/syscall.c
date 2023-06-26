#include "syscall.h"

void _exit(int status)
{
	syscall(SYS__EXIT, status);
	for (;;);
}

