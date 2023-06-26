#include "syscall.h"

void _exit(int status)
{
	syscall(SYS__EXIT, status);
	for (;;);
}

pid_t getpid(void)
{
	return syscall(SYS_GETPID);
}

void debug_printint(int64_t num)
{
	syscall(SYS_DEBUG_PRINTINT, num);
}

