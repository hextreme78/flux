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

	if (mkfifo("/fifine", 0777) < 0) {
		sys_printf("test\n");
		return 0;
	}
	/*
	char str1[] = "Hello, world!\n";
	char str0[] = ".............\n";
	int fds[2];

	if (pipe2(fds, 0) < 0) {
		sys_printf("error: pipe2() errno=%d\n", errno);
		return 0;
	}

	write(fds[1], str0, sizeof(str0));
	read(fds[0], str1, sizeof(str0));

	sys_printf("%s", str1);

	close(fds[0]);
	close(fds[1]);
	*/

	/*
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
	*/


	sys_printf("success\n");
	
	return 0;
}

