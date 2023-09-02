#ifndef KERNEL_FS_H
#define KERNEL_FS_H

#include <kernel/types.h>

#define O_RDONLY   0
#define O_WRONLY   1
#define O_RDWR     2
#define O_APPEND   0x0008
#define O_CREAT    0x0200
#define O_TRUNC    0x0400
#define O_EXCL     0x0800
#define O_NONBLOCK 0x4000
#define O_NOCTTY   0x8000

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

#endif

