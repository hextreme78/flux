#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <sys/types.h>

/* debug syscall */
#define SYS_DEBUG_PRINTINT 100

/* process syscalls */
#define SYS__EXIT  0
#define SYS_WAIT   1
#define SYS_KILL   2
#define SYS_GETPID 3
#define SYS_FORK   4
#define SYS_EXECVE 5
#define SYS_SBRK   6

/* filesystem syscalls */
#define SYS_CREAT    20
#define SYS_LINK     21
#define SYS_MKDIR    22
#define SYS_UNLINK   23
#define SYS_RMDIR    24
#define SYS_RENAME   25
#define SYS_READLINK 26
#define SYS_OPEN     27
#define SYS_READ     28
#define SYS_WRITE    29
#define SYS_LSEEK    30
#define SYS_CLOSE    31
#define SYS_STAT     32
#define SYS_CHDIR    33

#define O_RDONLY   1
#define O_WRONLY   2
#define O_RDWR     3
#define O_APPEND   4
#define O_CREAT    8
#define O_DSYNC    16
#define O_EXCL     32
#define O_NOCTTY   64
#define O_NONBLOCK 128
#define O_RSYNC    256
#define O_SYNC     512
#define O_TRUNC    1024

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct stat {
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	dev_t st_rdev;
	off_t st_size;
	time_t st_atime;
	time_t st_mtime;
	time_t st_ctime;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
};

long syscall(long number, ...);

void debug_printint(int64_t num);

void _exit(int status);
pid_t getpid(void);

int open(const char *path, int flags, ...);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
ssize_t lseek(int fd, off_t offset, int whence);
int close(int fd);
int creat(const char *path, mode_t flags);
int link(const char *old, const char *new);
int mkdir(const char *path, mode_t mode);
int unlink(const char *path);
int rmdir(const char *path);
int rename(const char *old, const char *new);
ssize_t readlink(const char *path, char *buf, size_t sz);
int stat(const char *path, struct stat *st);
int chdir(const char *path);

#endif

