#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdarg.h>

#include <bits/syscall.h>

void sys_printf(const char *fmt, ...)
{
	char buf[256];
	va_list args;
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	syscall(0, buf);
}

int main()
{
	sys_printf("Starting test\n");

	//int fd = open("/testfile", O_RDWR);

	//sys_printf("%d\n", fd);

	unlink("/testfile");

	//close(fd);

	/*
	int fds[2];
	fds[0] = open("/testfifo", O_RDWR);
	fds[1] = open("/testfifo", O_RDWR);

	char buf[100];
	write(fds[1], "test\n", 6);
	int a = read(fds[0], buf, 10);

	close(fds[0]);
	close(fds[1]);

	sys_printf("a = %d\n", a);
	sys_printf(buf);
	*/
/*
	int ret;
	int fd = open("/testfile", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	if (fd < 0) {
		sys_printf("open() error %d\n", fd);
		return -1;
	}

	char str[] = "write test\n";
	ssize_t nbytes = write(fd, str, sizeof(str));
	if (nbytes < 0) {
		sys_printf("write() error %d\n", nbytes);
		return -1;
	}
*/
/*
	int ret;
	int fd = open("/testfile", O_RDWR);
	if (fd < 0) {
		sys_printf("open() error %d\n", fd);
		return -1;
	}
	
	struct stat st;
	if ((ret = stat("/testfile", &st))) {
		sys_printf("stat() error %d\n", ret);
		return -1;
	}

	sys_printf("st_dev     = %d\n", st.st_dev);
	sys_printf("st_ino     = %d\n", st.st_ino);
	sys_printf("st_mode    = %x\n", st.st_mode);
	sys_printf("st_nlink   = %d\n", st.st_nlink);
	sys_printf("st_uid     = %d\n", st.st_uid);
	sys_printf("st_gid     = %d\n", st.st_gid);
	sys_printf("st_rdev    = %d\n", st.st_rdev);
	sys_printf("st_size    = %d\n", st.st_size);
	sys_printf("st_blksize = %d\n", st.st_blksize);
	sys_printf("st_blocks  = %d\n", st.st_blocks);
	
*/	
	sys_printf("Success\n");
	
	return 0;
}

