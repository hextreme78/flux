#include <kernel/ext2.h>
#include <kernel/alloc.h>
#include <kernel/virtio-blk.h>
#include <kernel/kprintf.h>

extern virtio_blk_t virtio_blk_list[VIRTIO_MAX];
ext2_blkdev_t ext2_dev_list;

int ext2_superblock_read(ext2_blkdev_t *blkdev)
{
	int err;
	/* read superblock sectors */
	err = virtio_blk_read(blkdev->virtio_devnum,
			EXT2_SUPERBLOCK_SECTOR0,
			&blkdev->superblock);
	if (err) {
		return err;
	}
	err = virtio_blk_read(blkdev->virtio_devnum,
			EXT2_SUPERBLOCK_SECTOR1,
			(u8 *) &blkdev->superblock + VIRTIO_BLK_SECTOR_SIZE);
	if (err) {
		return err;
	}
	return 0;
}

void ext2_init(void)
{
	int err;
	list_init(&ext2_dev_list.devlist);

	for (size_t i = 0; i < VIRTIO_MAX; i++)	{
		ext2_blkdev_t *blkdev;

		if (virtio_blk_list[i].isvalid) {
			blkdev = kmalloc(sizeof(*blkdev));

			blkdev->virtio_devnum = i;

			err = ext2_superblock_read(blkdev);
			if (err) {
				kfree(blkdev);
				continue;
			}

			/* check ext2 magic value */
			if (blkdev->superblock.s_magic != EXT2_SUPER_MAGIC) {
				kfree(blkdev);
				continue;
			}

			blkdev->blksz = 1024 << blkdev->superblock.s_log_block_size;

			list_add(&blkdev->devlist, &ext2_dev_list.devlist);
		}
	}
}

int ext2_blk_read(ext2_blkdev_t *dev, u64 blknum, void *buf)
{

	return 0;
}

int ext2_blk_write(ext2_blkdev_t *dev, u64 blknum, void *buf)
{

	return 0;
}

