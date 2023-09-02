#include <syscall.h>
#include <sys/types.h>

pid_t _getpid(void)
{
	return syscall(SYS_getpid);
}

