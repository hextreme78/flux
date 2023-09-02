#include <syscall.h>
#include <sys/types.h>

pid_t getpid(void)
{
	return syscall(SYS_getpid);
}

