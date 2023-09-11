#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

void sys_printf(const char *fmt, ...)
{
	char buf[256];
	va_list args;
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	syscall(1000, buf);
}

int main(void)
{
	if (chmod("/", 0777) < 0) {
		sys_printf("error: 1. chmod() errno=%d\n", errno);
		return 0;
	}

	if (mkdir("testdir", 0000) < 0) {
		sys_printf("error: mkdir() errno=%d\n", errno);
		return 0;
	}

	if (chdir("testdir") < 0) {
		sys_printf("error: 1. chdir() errno=%d\n", errno);
	} else {
		chdir("..");
	}

	if (chmod("testdir", 0700) < 0) {
		sys_printf("error: 2. chmod() errno=%d\n", errno);
		return 0;
	}

	if (chdir("testdir") < 0) {
		sys_printf("error: 2. chdir() errno=%d\n", errno);
	}

	if (creat("testreg", 0070) < 0) {
		sys_printf("error: creat() errno=%d\n", errno);
		return 0;
	}

	char cwd[128];
	if (!getcwd(cwd, sizeof(cwd))) {
		sys_printf("error: getcwd() errno=%d\n", errno);
		return 0;
	}

	sys_printf("%s\n", cwd);

	sys_printf("success\n");

	return 0;
}

