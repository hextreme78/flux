#ifndef KERNEL_FS_H
#define KERNEL_FS_H

#include <kernel/types.h>

#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)

#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2
#define O_APPEND    0x0008
#define O_CREAT     0x0200
#define O_TRUNC     0x0400
#define O_EXCL      0x0800
#define O_SYNC      0x2000
#define O_NONBLOCK  0x4000
#define O_NOCTTY    0x8000
#define	O_CLOEXEC   0x40000
#define	O_NOFOLLOW  0x100000
#define	O_DIRECTORY 0x200000
#define	O_EXEC      0x400000
#define	O_SEARCH    O_EXEC
#define	O_DIRECT    0x80000
#define O_BINARY    0x10000
#define O_TEXT      0x20000
#define O_TMPFILE   0x800000
#define O_NOATIME   0x1000000
#define O_PATH      0x2000000

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define	F_DUPFD		0
#define	F_GETFD		1
#define	F_SETFD		2
#define	F_GETFL		3
#define	F_SETFL		4
#define	F_GETOWN 	5
#define	F_SETOWN 	6
#define	F_GETLK  	7
#define	F_SETLK  	8
#define	F_SETLKW 	9
#define	F_RGETLK 	10
#define	F_RSETLK 	11
#define	F_CNVT 		12
#define	F_RSETLKW 	13
#define	F_DUPFD_CLOEXEC	14

#define	FD_CLOEXEC 1

#define	S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)
#define	S_IRUSR	0000400
#define	S_IWUSR	0000200
#define	S_IXUSR 0000100
#define	S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP)
#define	S_IRGRP	0000040
#define	S_IWGRP	0000020
#define	S_IXGRP 0000010
#define	S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH)
#define	S_IROTH	0000004
#define	S_IWOTH	0000002
#define	S_IXOTH 0000001

#define F_OK 0
#define	R_OK 4
#define	W_OK 2
#define	X_OK 1

#define	S_IFMT   0170000
#define	S_IFDIR  0040000
#define	S_IFCHR  0020000
#define	S_IFBLK  0060000
#define	S_IFREG  0100000
#define	S_IFLNK  0120000
#define	S_IFSOCK 0140000
#define	S_IFIFO  0010000

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

#endif

