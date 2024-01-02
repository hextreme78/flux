#ifndef KERNEL_FS_H
#define KERNEL_FS_H

typedef struct opened_inode opened_inode_t;
typedef struct fifodesc     fifodesc_t;

#include <abi-bits/fcntl.h> 
#include <abi-bits/access.h>
#include <abi-bits/seek-whence.h>
#include <abi-bits/stat.h>

#include <kernel/list.h>
#include <kernel/spinlock.h>

extern spinlock_t opened_inodes_lock;
extern opened_inode_t opened_inodes;
extern spinlock_t fifodescs_lock;
extern fifodesc_t fifodescs;

struct opened_inode {
	ino_t inum;
	bool deletemark;
	int refcnt;
	list_t opened_inodes_list;
};

struct fifodesc {
	ino_t inum;
	off_t roffset;
	off_t woffset;
	void *pipebuf;
	list_t fifodescs_list;
};

static inline int opened_inodes_cmp(void *a, void *b)
{
	ino_t inuma = ((opened_inode_t *) a)->inum;
	ino_t inumb = ((opened_inode_t *) b)->inum;
	if (inuma > inumb) {
		return 1;
	} else if (inuma < inumb) {
		return -1;
	}
	return 0;
}

static inline int fifodescs_cmp(void *a, void *b)
{
	ino_t inuma = ((fifodesc_t *) a)->inum;
	ino_t inumb = ((fifodesc_t *) b)->inum;
	if (inuma > inumb) {
		return 1;
	} else if (inuma < inumb) {
		return -1;
	}
	return 0;
}

void fs_init(void);

#endif

