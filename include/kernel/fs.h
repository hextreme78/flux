#ifndef KERNEL_FS_H
#define KERNEL_FS_H

#include <kernel/types.h>

#define O_RDONLY   1
#define O_WRONLY   2
#define O_RDWR     (O_RDONLY | O_WRONLY)
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

#endif

