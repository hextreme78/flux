#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

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

static inline dev_t makedev(int major, int minor)
{
	return ((major & 0xff) << 8) | (minor & 0xff);
}

int main()
{
	sys_printf("Starting test\n");

	/*
	mkdir("/dev", 0777);
	mknod("/dev/zero", S_IFCHR | 0777, makedev(1, 4));
	*/
	
	int ret;
	int fd = open("/dev/zero", O_RDWR);
	if (fd < 0) {
		sys_printf("open() error %d\n", fd);
		return -1;
	}

	char buf[128] = "Hello, world!!!\n";
	ret = read(fd, buf, strlen(buf));
	sys_printf("str is %s\n", buf);
	sys_printf("%d\n", ret);

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

