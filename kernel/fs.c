#include <kernel/fs.h>
#include <kernel/ext2.h>

spinlock_t opened_inodes_lock;
opened_inode_t opened_inodes;
spinlock_t fifodescs_lock;
fifodesc_t fifodescs;

void fs_init(void)
{
	spinlock_init(&opened_inodes_lock);
	list_init(&opened_inodes.opened_inodes_list);
	spinlock_init(&fifodescs_lock);
	list_init(&fifodescs.fifodescs_list);
	ext2_init();
	ext2_root_mount();
}

