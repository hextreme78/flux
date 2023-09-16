#include <kernel/ext2.h>
#include <kernel/alloc.h>
#include <kernel/virtio-blk.h>
#include <kernel/kprintf.h>
#include <kernel/klib.h>
#include <kernel/errno.h>

ext2_blkdev_t *rootblkdev;
ext2_blkdev_t ext2_dev_list;

static int ext2_init_superblock_read(ext2_superblock_t *superblock,
		size_t virtio_devnum)
{
	int err = 0;
	err = virtio_blk_read_nosleep(virtio_devnum,
			EXT2_SUPERBLOCK_START / VIRTIO_BLK_SECTOR_SIZE,
			superblock);
	return err;
}

static int ext2_init_superblock_write(ext2_superblock_t *superblock,
		size_t virtio_devnum)
{
	int err = 0;
	err = virtio_blk_write_nosleep(virtio_devnum,
			EXT2_SUPERBLOCK_START / VIRTIO_BLK_SECTOR_SIZE,
			superblock);
	return err;
}

void ext2_root_mount(void)
{
	int err;
	ext2_superblock_t superblock;

	rootblkdev = list_next_entry(&ext2_dev_list, devlist);

	err = ext2_init_superblock_read(&superblock, rootblkdev->virtio_devnum);
	if (err) {
		panic("ext2_root_mount(): ext2_init_superblock_read() failed");
	}

	superblock.s_mtime = 0;
	superblock.s_mnt_count++;
	if (superblock.s_state == EXT2_ERROR_FS) {
		kprintf_s("rootfs was not cleanly unmounted last time. "
				"fsck is needed.\n");
	}
	superblock.s_state = EXT2_ERROR_FS;

	err = ext2_init_superblock_write(&superblock, rootblkdev->virtio_devnum);
	if (err) {
		panic("ext2_root_mount(): ext2_init_superblock_write() failed");
	}

}

void ext2_root_umount(void)
{
	/* not implemented */

}

void ext2_init(void)
{
	extern virtio_blk_t virtio_blk_list[VIRTIO_MAX];
	int err;
	ext2_superblock_t superblock;

	list_init(&ext2_dev_list.devlist);

	for (size_t i = 0; i < VIRTIO_MAX; i++)	{
		if (virtio_blk_list[i].isvalid) {
			ext2_blkdev_t *blkdev;
			err = ext2_init_superblock_read(&superblock, i);
			if (err) {
				continue;
			}

			/* check ext2 magic value */
			if (superblock.s_magic != EXT2_SUPER_MAGIC) {
				continue;
			}

			blkdev = kmalloc(ALIGNED_ALLOC_SZ(sizeof(*blkdev), 8));
			blkdev = ALIGNED_ALLOC_PTR(blkdev, 8);

			blkdev->virtio_devnum = i;
			blkdev->block_size = 1024 << superblock.s_log_block_size;
			blkdev->inodes_count = superblock.s_inodes_count;
			blkdev->blocks_count = superblock.s_blocks_count;
			blkdev->inodes_per_group = superblock.s_inodes_per_group;
			blkdev->blocks_per_group = superblock.s_blocks_per_group;

			blkdev->blockgroups_count =
				blkdev->inodes_count % blkdev->inodes_per_group ?
				blkdev->inodes_count / blkdev->inodes_per_group + 1 :
				blkdev->inodes_count / blkdev->inodes_per_group;

			blkdev->rev_level = superblock.s_rev_level;

			if (superblock.s_rev_level == EXT2_GOOD_OLD_REV) {
				blkdev->inode_size = EXT2_INODE_SIZE;
			} else {
				blkdev->inode_size = superblock.s_inode_size;
			}

			mutex_init(&blkdev->lock);

			list_add(&blkdev->devlist, &ext2_dev_list.devlist);
		}
	}
}

static int ext2_block_read(ext2_blkdev_t *dev, u64 blknum, void *buf)
{
	int err;
	for (size_t i = 0; i < dev->block_size / VIRTIO_BLK_SECTOR_SIZE; i++) {
		err = virtio_blk_read(dev->virtio_devnum,
				blknum * dev->block_size / VIRTIO_BLK_SECTOR_SIZE + i,
				buf + i * VIRTIO_BLK_SECTOR_SIZE);
		if (err) {
			return err;
		}
	}
	return 0;
}

static int ext2_block_write(ext2_blkdev_t *dev, u64 blknum, void *buf)
{
	int err;
	for (size_t i = 0; i < dev->block_size / VIRTIO_BLK_SECTOR_SIZE; i++) {
		err = virtio_blk_write(dev->virtio_devnum,
				blknum * dev->block_size / VIRTIO_BLK_SECTOR_SIZE + i,
				buf + i * VIRTIO_BLK_SECTOR_SIZE);
		if (err) {
			return err;
		}
	}
	return 0;
}

int ext2_nbytes_read(ext2_blkdev_t *dev, void *buf, u64 len, u64 offset)
{
	int err;
	u8 blockbuf[dev->block_size];
	u64 firstblock = offset / dev->block_size;
	u64 lastblock = (offset + len) / dev->block_size;
	u64 ncopied = 0;
	u64 inblock_off, inblock_len;

	for (u64 curblock = firstblock; curblock <= lastblock; curblock++) {
		if (curblock == firstblock) {
			inblock_off = offset - curblock * dev->block_size;
			if (dev->block_size - inblock_off > len) {
				inblock_len = len;
			} else {
				inblock_len = dev->block_size - inblock_off;
			}
		} else if (curblock == lastblock) {
			inblock_off = 0;
			inblock_len = len - ncopied;
		} else {
			inblock_off = 0;
			inblock_len = dev->block_size;
		}

		err = ext2_block_read(dev, curblock, blockbuf);
		if (err) {
			return err;
		}

		memcpy((u8 *) buf + ncopied, blockbuf + inblock_off, inblock_len);

		ncopied += inblock_len;
	}

	return 0;
}

int ext2_nbytes_write(ext2_blkdev_t *dev, void *buf, u64 len, u64 offset)
{
	int err;
	u8 blockbuf[dev->block_size];
	u64 firstblock = offset / dev->block_size;
	u64 lastblock = (offset + len) / dev->block_size;
	u64 ncopied = 0;
	u64 inblock_off, inblock_len;

	for (u64 curblock = firstblock; curblock <= lastblock; curblock++) {
		if (curblock == firstblock) {
			inblock_off = offset - curblock * dev->block_size;
			if (dev->block_size - inblock_off > len) {
				inblock_len = len;
			} else {
				inblock_len = dev->block_size - inblock_off;
			}

			err = ext2_block_read(dev, curblock, blockbuf);
			if (err) {
				return err;
			}
		} else if (curblock == lastblock) {
			inblock_off = 0;
			inblock_len = len - ncopied;
		} else {
			inblock_off = 0;
			inblock_len = dev->block_size;

			err = ext2_block_read(dev, curblock, blockbuf);
			if (err) {
				return err;
			}
		}

		memcpy(blockbuf + inblock_off, (u8 *) buf + ncopied, inblock_len);

		err = ext2_block_write(dev, curblock, blockbuf);
		if (err) {
			return err;
		}

		ncopied += inblock_len;
	}

	return 0;
}

static int ext2_superblock_read(ext2_blkdev_t *dev, ext2_superblock_t *superblock)
{
	int err;

	err = ext2_nbytes_read(dev, superblock, sizeof(*superblock),
			EXT2_SUPERBLOCK_START);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_superblock_write(ext2_blkdev_t *dev, ext2_superblock_t *superblock)
{
	int err;

	err = ext2_nbytes_write(dev, superblock, sizeof(*superblock),
			EXT2_SUPERBLOCK_START);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_blockgroup_descriptor_read(ext2_blkdev_t *dev, u32 bgnum,
		ext2_blockgroup_descriptor_t *bgdesc)
{
	int err;

	err = ext2_nbytes_read(dev, bgdesc, sizeof(*bgdesc),
			EXT2_BLOCKGROUP_DESCRIPTOR_TABLE_START +
			bgnum * sizeof(*bgdesc));
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_blockgroup_descriptor_write(ext2_blkdev_t *dev, u32 bgnum,
		ext2_blockgroup_descriptor_t *bgdesc)
{
	int err;

	err = ext2_nbytes_write(dev, bgdesc, sizeof(*bgdesc),
			EXT2_BLOCKGROUP_DESCRIPTOR_TABLE_START +
			bgnum * sizeof(*bgdesc));
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_inode_read(ext2_blkdev_t *dev, u32 inum, ext2_inode_t *inode)
{
	int err;
	u32 bgnum = (inum - 1) / dev->inodes_per_group;
	u32 index = (inum - 1) % dev->inodes_per_group;
	ext2_blockgroup_descriptor_t bgdesc;

	err = ext2_blockgroup_descriptor_read(dev, bgnum, &bgdesc);
	if (err) {
		return err;
	}

	err = ext2_nbytes_read(dev, inode, sizeof(*inode),
			bgdesc.bg_inode_table * dev->block_size +
			index * dev->inode_size);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_inode_write(ext2_blkdev_t *dev, u32 inum, ext2_inode_t *inode)
{
	int err;
	u32 bgnum = (inum - 1) / dev->inodes_per_group;
	u32 index = (inum - 1) % dev->inodes_per_group;
	ext2_blockgroup_descriptor_t bgdesc;

	err = ext2_blockgroup_descriptor_read(dev, bgnum, &bgdesc);
	if (err) {
		return err;
	}
	
	err = ext2_nbytes_write(dev, inode, sizeof(*inode),
			bgdesc.bg_inode_table * dev->block_size +
			index * dev->inode_size);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_inode_counter_increment(ext2_blkdev_t *dev, u32 inum)
{
	int err;
	u32 bgnum = (inum - 1) / dev->inodes_per_group;
	ext2_blockgroup_descriptor_t bgdesc;
	ext2_superblock_t superblock;

	err = ext2_blockgroup_descriptor_read(dev, bgnum, &bgdesc);
	if (err) {
		return err;
	}
	bgdesc.bg_free_inodes_count++;
	err = ext2_blockgroup_descriptor_write(dev, bgnum, &bgdesc);
	if (err) {
		return err;
	}

	err = ext2_superblock_read(dev, &superblock);
	if (err) {
		return err;
	}
	superblock.s_free_inodes_count++;
	err = ext2_superblock_write(dev, &superblock);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_inode_counter_decrement(ext2_blkdev_t *dev, u32 inum)
{
	int err;
	u32 bgnum = (inum - 1) / dev->inodes_per_group;
	ext2_blockgroup_descriptor_t bgdesc;
	ext2_superblock_t superblock;

	err = ext2_blockgroup_descriptor_read(dev, bgnum, &bgdesc);
	if (err) {
		return err;
	}
	bgdesc.bg_free_inodes_count--;
	err = ext2_blockgroup_descriptor_write(dev, bgnum, &bgdesc);
	if (err) {
		return err;
	}

	err = ext2_superblock_read(dev, &superblock);
	if (err) {
		return err;
	}
	superblock.s_free_inodes_count--;
	err = ext2_superblock_write(dev, &superblock);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_inode_allocate(ext2_blkdev_t *dev, u32 *inum)
{
	int err;
	ext2_blockgroup_descriptor_t bgdesc;
	char inode_bitmap[dev->block_size];

	for (size_t bgnum = 0; bgnum < dev->blockgroups_count; bgnum++) {
		err = ext2_blockgroup_descriptor_read(dev, bgnum, &bgdesc);
		if (err) {
			return err;
		}

		err = ext2_block_read(dev, bgdesc.bg_inode_bitmap, inode_bitmap);
		if (err) {
			return err;
		}

		for (size_t i = 0; i < dev->inodes_per_group; i++) {
			if (!(inode_bitmap[i / 8] & (1 << (i % 8)))) {
				inode_bitmap[i / 8] |= 1 << (i % 8);

				err = ext2_block_write(dev, bgdesc.bg_inode_bitmap,
						inode_bitmap);
				if (err) {
					return err;
				}

				*inum = bgnum * dev->inodes_per_group + i + 1;

				err = ext2_inode_counter_decrement(dev, *inum);
				if (err) {
					return err;
				}

				return 0;
			}
		}
	}

	return -ENOSPC;
}

static int ext2_inode_free(ext2_blkdev_t *dev, u32 inum)
{
	int err;
	u32 bgnum = (inum - 1) / dev->inodes_per_group;
	u32 index = (inum - 1) % dev->inodes_per_group;
	ext2_blockgroup_descriptor_t bgdesc;
	char inode_bitmap[dev->block_size];

	err = ext2_blockgroup_descriptor_read(dev, bgnum, &bgdesc);
	if (err) {
		return err;
	}

	err = ext2_block_read(dev, bgdesc.bg_inode_bitmap, inode_bitmap);
	if (err) {
		return err;
	}

	inode_bitmap[index / 8] &= ~(1 << (index % 8));

	err = ext2_block_write(dev, bgdesc.bg_inode_bitmap,
			inode_bitmap);
	if (err) {
		return err;
	}

	err = ext2_inode_counter_increment(dev, inum);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_block_counter_increment(ext2_blkdev_t *dev, u32 blknum)
{
	int err;
	u32 bgnum = blknum / dev->blocks_per_group;
	ext2_blockgroup_descriptor_t bgdesc;
	ext2_superblock_t superblock;

	err = ext2_blockgroup_descriptor_read(dev, bgnum, &bgdesc);
	if (err) {
		return err;
	}
	bgdesc.bg_free_blocks_count++;
	err = ext2_blockgroup_descriptor_write(dev, bgnum, &bgdesc);
	if (err) {
		return err;
	}

	err = ext2_superblock_read(dev, &superblock);
	if (err) {
		return err;
	}
	superblock.s_free_blocks_count++;
	err = ext2_superblock_write(dev, &superblock);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_block_counter_decrement(ext2_blkdev_t *dev, u32 blknum)
{
	int err;
	u32 bgnum = blknum / dev->blocks_per_group;
	ext2_blockgroup_descriptor_t bgdesc;
	ext2_superblock_t superblock;

	err = ext2_blockgroup_descriptor_read(dev, bgnum, &bgdesc);
	if (err) {
		return err;
	}
	bgdesc.bg_free_blocks_count--;
	err = ext2_blockgroup_descriptor_write(dev, bgnum, &bgdesc);
	if (err) {
		return err;
	}

	err = ext2_superblock_read(dev, &superblock);
	if (err) {
		return err;
	}
	superblock.s_free_blocks_count--;
	err = ext2_superblock_write(dev, &superblock);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_block_allocate(ext2_blkdev_t *dev, u32 *blknum, u32 inode_hint)
{
	int err;
	ext2_blockgroup_descriptor_t bgdesc;
	char block_bitmap[dev->block_size];
	size_t bgnum_hint = (inode_hint - 1) / dev->inodes_per_group;

	/* First we should try to allocate block in the same 
	 * blockgroup in which inode is located.
	 */
	err = ext2_blockgroup_descriptor_read(dev, bgnum_hint, &bgdesc);
	if (err) {
		return err;
	}
	err = ext2_block_read(dev, bgdesc.bg_block_bitmap, block_bitmap);
	if (err) {
		return err;
	}
	for (size_t i = 0; i < dev->blocks_per_group; i++) {
		if (!(block_bitmap[i / 8] & (1 << (i % 8)))) {
			block_bitmap[i / 8] |= 1 << (i % 8);

			err = ext2_block_write(dev, bgdesc.bg_block_bitmap,
					block_bitmap);
			if (err) {
				return err;
			}

			*blknum = bgnum_hint * dev->blocks_per_group + i;

			err = ext2_block_counter_decrement(dev, *blknum);
			if (err) {
				return err;
			}

			return 0;
		}
	}

	/* If inode's blockgroup does not contain free blocks 
	 * try to search for free block in all blockgroups.
	 */
	for (size_t bgnum = 0; bgnum < dev->blockgroups_count; bgnum++) {
		if (bgnum == bgnum_hint) {
			continue;
		}

		err = ext2_blockgroup_descriptor_read(dev, bgnum, &bgdesc);
		if (err) {
			return err;
		}

		err = ext2_block_read(dev, bgdesc.bg_block_bitmap, block_bitmap);
		if (err) {
			return err;
		}

		for (size_t i = 0; i < dev->blocks_per_group; i++) {
			if (!(block_bitmap[i / 8] & (1 << (i % 8)))) {
				block_bitmap[i / 8] |= 1 << (i % 8);

				err = ext2_block_write(dev, bgdesc.bg_block_bitmap,
						block_bitmap);
				if (err) {
					return err;
				}

				*blknum = bgnum * dev->blocks_per_group + i;
				
				err = ext2_block_counter_decrement(dev, *blknum);
				if (err) {
					return err;
				}

				return 0;
			}
		}
	}

	return -ENOSPC;
}

static int ext2_block_free(ext2_blkdev_t *dev, u32 blknum)
{
	int err;
	u32 bgnum = blknum / dev->blocks_per_group;
	u32 index = blknum % dev->blocks_per_group;
	ext2_blockgroup_descriptor_t bgdesc;
	char block_bitmap[dev->block_size];

	err = ext2_blockgroup_descriptor_read(dev, bgnum, &bgdesc);
	if (err) {
		return err;
	}

	err = ext2_block_read(dev, bgdesc.bg_block_bitmap, block_bitmap);
	if (err) {
		return err;
	}

	block_bitmap[index / 8] &= ~(1 << (index % 8));

	err = ext2_block_write(dev, bgdesc.bg_block_bitmap,
			block_bitmap);
	if (err) {
		return err;
	}

	err = ext2_block_counter_increment(dev, blknum);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_file_block_allocate(ext2_blkdev_t *dev, u32 inum, u32 blknum)
{
	int err;
	ext2_inode_t inode;
	u32 blockbuf0[dev->block_size / sizeof(u32)];
	u32 blockbuf1[dev->block_size / sizeof(u32)];
	u32 blockbuf2[dev->block_size / sizeof(u32)];
	u32 blknum0, blknum1, blknum2;
	u8 zeroblockbuf[dev->block_size];
	u64 singly_indirect = dev->block_size / sizeof(*inode.i_block);
	u64 doubly_indirect = singly_indirect * singly_indirect;
	u64 triply_indirect = doubly_indirect * singly_indirect;

	bzero(zeroblockbuf, dev->block_size);

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	if (blknum < 12) {
		if (inode.i_block[blknum]) {
			return -EINVAL;
		}
		err = ext2_block_allocate(dev, &inode.i_block[blknum], inum);
		if (err) {
			return err;
		}
		inode.i_blocks++;
	} else if (blknum < 12 + singly_indirect) {
		if (!inode.i_block[12]) {
			err = ext2_block_allocate(dev, &inode.i_block[12], inum);
			if (err) {
				return err;
			}
			err = ext2_block_write(dev, inode.i_block[12], zeroblockbuf);
			if (err) {
				return err;
			}
			inode.i_blocks++;
		}

		err = ext2_block_read(dev, inode.i_block[12], blockbuf0);
		if (err) {
			return err;
		}
		if (blockbuf0[blknum - 12]) {
			return -EINVAL;
		}
		err = ext2_block_allocate(dev, &blockbuf0[blknum - 12], inum);
		if (err) {
			return err;
		}
		err = ext2_block_write(dev, inode.i_block[12], blockbuf0);
		if (err) {
			return err;
		}
		inode.i_blocks++;
	} else if (blknum < 12 + singly_indirect + doubly_indirect) {
		blknum0 = (blknum - 12 - singly_indirect) / singly_indirect;
		blknum1 = (blknum - 12 - singly_indirect) - blknum0 * singly_indirect;
		if (!inode.i_block[13]) {
			err = ext2_block_allocate(dev, &inode.i_block[13], inum);
			if (err) {
				return err;
			}
			err = ext2_block_write(dev, inode.i_block[13], zeroblockbuf);
			if (err) {
				return err;
			}
			inode.i_blocks++;
		}

		err = ext2_block_read(dev, inode.i_block[13], blockbuf0);
		if (err) {
			return err;
		}
		if (!blockbuf0[blknum0]) {
			err = ext2_block_allocate(dev, &blockbuf0[blknum0], inum);
			if (err) {
				return err;
			}
			err = ext2_block_write(dev, blockbuf0[blknum0], zeroblockbuf);
			if (err) {
				return err;
			}
			err = ext2_block_write(dev, inode.i_block[13], blockbuf0);
			if (err) {
				return err;
			}
			inode.i_blocks++;
		}

		err = ext2_block_read(dev, blockbuf0[blknum0], blockbuf1);
		if (err) {
			return err;
		}
		if (blockbuf1[blknum1]) {
			return -EINVAL;
		}
		err = ext2_block_allocate(dev, &blockbuf1[blknum1], inum);
		if (err) {
			return err;
		}
		err = ext2_block_write(dev, blockbuf0[blknum0], blockbuf1);
		if (err) {
			return err;
		}
		inode.i_blocks++;
	} else if (blknum < 12 + singly_indirect + doubly_indirect +
			triply_indirect) {
		blknum0 = (blknum - 12 - singly_indirect - doubly_indirect) /
			doubly_indirect;
		blknum1 = (blknum - 12 - singly_indirect - doubly_indirect) /
			singly_indirect - blknum0 * singly_indirect;
		blknum2 = (blknum - 12 - singly_indirect - doubly_indirect) -
			((blknum - 12 - singly_indirect - doubly_indirect) /
			 singly_indirect) * singly_indirect;
		if (!inode.i_block[14]) {
			err = ext2_block_allocate(dev, &inode.i_block[14], inum);
			if (err) {
				return err;
			}
			err = ext2_block_write(dev, inode.i_block[14], zeroblockbuf);
			if (err) {
				return err;
			}
			inode.i_blocks++;
		}

		err = ext2_block_read(dev, inode.i_block[14], blockbuf0);
		if (err) {
			return err;
		}
		if (!blockbuf0[blknum0]) {
			err = ext2_block_allocate(dev, &blockbuf0[blknum0], inum);
			if (err) {
				return err;
			}
			err = ext2_block_write(dev, blockbuf0[blknum0], zeroblockbuf);
			if (err) {
				return err;
			}
			err = ext2_block_write(dev, inode.i_block[14], blockbuf0);
			if (err) {
				return err;
			}
			inode.i_blocks++;
		}

		err = ext2_block_read(dev, blockbuf0[blknum0], blockbuf1);
		if (err) {
			return err;
		}
		if (!blockbuf1[blknum1]) {
			err = ext2_block_allocate(dev, &blockbuf1[blknum1], inum);
			if (err) {
				return err;
			}
			err = ext2_block_write(dev, blockbuf1[blknum1], zeroblockbuf);
			if (err) {
				return err;
			}
			err = ext2_block_write(dev, blockbuf0[blknum0], blockbuf1);
			if (err) {
				return err;
			}
			inode.i_blocks++;	
		}

		err = ext2_block_read(dev, blockbuf1[blknum1], blockbuf2);
		if (err) {
			return err;
		}
		if (blockbuf2[blknum2]) {
			return -EINVAL;
		}
		err = ext2_block_allocate(dev, &blockbuf2[blknum2], inum);
		if (err) {
			return err;
		}
		err = ext2_block_write(dev, blockbuf1[blknum1], blockbuf2);
		if (err) {
			return err;
		}
		inode.i_blocks++;
	} else {
		return -EFBIG;
	}

	err = ext2_inode_write(dev, inum, &inode);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_file_block_free(ext2_blkdev_t *dev, u32 inum, u32 blknum)
{
	int err;
	ext2_inode_t inode;
	u32 blockbuf0[dev->block_size / sizeof(u32)];
	u32 blockbuf1[dev->block_size / sizeof(u32)];
	u32 blockbuf2[dev->block_size / sizeof(u32)];
	u8 zeroblockbuf[dev->block_size];
	u32 blknum0, blknum1, blknum2;
	u64 singly_indirect = dev->block_size / sizeof(*inode.i_block);
	u64 doubly_indirect = singly_indirect * singly_indirect;
	u64 triply_indirect = doubly_indirect * singly_indirect;

	bzero(zeroblockbuf, dev->block_size);

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	if (blknum < 12) {
		if (!inode.i_block[blknum]) {
			return 0;
		}
		err = ext2_block_free(dev, inode.i_block[blknum]);
		if (err) {
			return err;
		}
		inode.i_block[blknum] = 0;
		inode.i_blocks--;
	} else if (blknum < 12 + singly_indirect) {
		if (!inode.i_block[12]) {
			return 0;
		}

		err = ext2_block_read(dev, inode.i_block[12], blockbuf0);
		if (err) {
			return err;
		}
		if (!blockbuf0[blknum - 12]) {
			return 0;
		}
		err = ext2_block_free(dev, blockbuf0[blknum - 12]);
		if (err) {
			return err;
		}
		blockbuf0[blknum - 12] = 0;
		inode.i_blocks--;
		err = ext2_block_write(dev, inode.i_block[12], blockbuf0);
		if (err) {
			return err;
		}

		if (!memcmp(blockbuf0, zeroblockbuf, dev->block_size)) {
			err = ext2_block_free(dev, inode.i_block[12]);
			if (err) {
				return err;
			}
			inode.i_block[12] = 0;
			inode.i_blocks--;
		}
	} else if (blknum < 12 + singly_indirect + doubly_indirect) {
		blknum0 = (blknum - 12 - singly_indirect) / singly_indirect;
		blknum1 = (blknum - 12 - singly_indirect) - blknum0 * singly_indirect;
		if (!inode.i_block[13]) {
			return 0;
		}

		err = ext2_block_read(dev, inode.i_block[13], blockbuf0);
		if (err) {
			return err;
		}
		if (!blockbuf0[blknum0]) {
			return 0;
		}

		err = ext2_block_read(dev, blockbuf0[blknum0], blockbuf1);
		if (err) {
			return err;
		}
		if (!blockbuf1[blknum1]) {
			return 0;
		}
		err = ext2_block_free(dev, blockbuf1[blknum1]);
		if (err) {
			return err;
		}
		blockbuf1[blknum1] = 0;
		inode.i_blocks--;
		err = ext2_block_write(dev, blockbuf0[blknum0], blockbuf1);
		if (err) {
			return err;
		}

		if (!memcmp(blockbuf1, zeroblockbuf, dev->block_size)) {
			err = ext2_block_free(dev, blockbuf0[blknum0]);
			if (err) {
				return err;
			}
			blockbuf0[blknum0] = 0;
			inode.i_blocks--;
			err = ext2_block_write(dev, inode.i_block[13], blockbuf0);
			if (err) {
				return err;
			}
		}
		if (!memcmp(blockbuf0, zeroblockbuf, dev->block_size)) {
			err = ext2_block_free(dev, inode.i_block[13]);
			if (err) {
				return err;
			}
			inode.i_block[13] = 0;
			inode.i_blocks--;
		}
	} else if (blknum < 12 + singly_indirect + doubly_indirect +
			triply_indirect) {
		blknum0 = (blknum - 12 - singly_indirect - doubly_indirect) /
			doubly_indirect;
		blknum1 = (blknum - 12 - singly_indirect - doubly_indirect) /
			singly_indirect - blknum0 * singly_indirect;
		blknum2 = (blknum - 12 - singly_indirect - doubly_indirect) -
			((blknum - 12 - singly_indirect - doubly_indirect) /
			 singly_indirect) * singly_indirect;
		if (!inode.i_block[14]) {
			return 0;
		}

		err = ext2_block_read(dev, inode.i_block[14], blockbuf0);
		if (err) {
			return err;
		}
		if (!blockbuf0[blknum0]) {
			return 0;
		}

		err = ext2_block_read(dev, blockbuf0[blknum0], blockbuf1);
		if (err) {
			return err;
		}
		if (!blockbuf1[blknum1]) {
			return 0;
		}

		err = ext2_block_read(dev, blockbuf1[blknum1], blockbuf2);
		if (err) {
			return err;
		}
		if (!blockbuf2[blknum2]) {
			return 0;
		}
		err = ext2_block_free(dev, blockbuf2[blknum2]);
		if (err) {
			return err;
		}
		blockbuf2[blknum2] = 0;
		inode.i_blocks--;
		err = ext2_block_write(dev, blockbuf1[blknum1], blockbuf2);
		if (err) {
			return err;
		}

		if (!memcmp(blockbuf2, zeroblockbuf, dev->block_size)) {
			err = ext2_block_free(dev, blockbuf1[blknum1]);
			if (err) {
				return err;
			}
			blockbuf1[blknum1] = 0;
			inode.i_blocks--;
			err = ext2_block_write(dev, blockbuf0[blknum0], blockbuf1);
			if (err) {
				return err;
			}
		}
		if (!memcmp(blockbuf1, zeroblockbuf, dev->block_size)) {
			err = ext2_block_free(dev, blockbuf0[blknum0]);
			if (err) {
				return err;
			}
			blockbuf0[blknum0] = 0;
			inode.i_blocks--;
			err = ext2_block_write(dev, inode.i_block[13], blockbuf0);
			if (err) {
				return err;
			}
		}
		if (!memcmp(blockbuf0, zeroblockbuf, dev->block_size)) {
			err = ext2_block_free(dev, inode.i_block[14]);
			if (err) {
				return err;
			}
			inode.i_block[14] = 0;
			inode.i_blocks--;
		}
	} else {
		return -EFBIG;
	}

	err = ext2_inode_write(dev, inum, &inode);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_file_block_read(ext2_blkdev_t *dev, u32 inum, u32 blknum,
		void *blockbuf)
{
	int err;
	ext2_inode_t inode;
	u32 blockbuf0[dev->block_size / sizeof(u32)];
	u32 blockbuf1[dev->block_size / sizeof(u32)];
	u32 blockbuf2[dev->block_size / sizeof(u32)];
	u32 blknum0, blknum1, blknum2;
	u64 singly_indirect = dev->block_size / sizeof(*inode.i_block);
	u64 doubly_indirect = singly_indirect * singly_indirect;
	u64 triply_indirect = doubly_indirect * singly_indirect;

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	if (blknum < 12) {
		if (!inode.i_block[blknum]) {
			bzero(blockbuf, dev->block_size);
			return 0;
		}
		err = ext2_block_read(dev, inode.i_block[blknum], blockbuf);
		if (err) {
			return err;
		}
	} else if (blknum < 12 + singly_indirect) {
		if (!inode.i_block[12]) {
			bzero(blockbuf, dev->block_size);
			return 0;
		}

		err = ext2_block_read(dev, inode.i_block[12], blockbuf0);
		if (err) {
			return err;
		}
		if (!blockbuf0[blknum - 12]) {
			bzero(blockbuf, dev->block_size);
			return 0;
		}
		err = ext2_block_read(dev, blockbuf0[blknum - 12], blockbuf);
		if (err) {
			return err;
		}
	} else if (blknum < 12 + singly_indirect + doubly_indirect) {
		blknum0 = (blknum - 12 - singly_indirect) / singly_indirect;
		blknum1 = (blknum - 12 - singly_indirect) - blknum0 * singly_indirect;
		if (!inode.i_block[13]) {
			bzero(blockbuf, dev->block_size);
			return 0;
		}

		err = ext2_block_read(dev, inode.i_block[13], blockbuf0);
		if (err) {
			return err;
		}
		if (!blockbuf0[blknum0]) {
			bzero(blockbuf, dev->block_size);
			return 0;
		}

		err = ext2_block_read(dev, blockbuf0[blknum0], blockbuf1);
		if (err) {
			return err;
		}
		if (!blockbuf1[blknum1]) {
			bzero(blockbuf, dev->block_size);
			return 0;
		}
		err = ext2_block_read(dev, blockbuf1[blknum1], blockbuf);
		if (err) {
			return err;
		}
	} else if (blknum < 12 + singly_indirect + doubly_indirect +
			triply_indirect) {
		blknum0 = (blknum - 12 - singly_indirect - doubly_indirect) /
			doubly_indirect;
		blknum1 = (blknum - 12 - singly_indirect - doubly_indirect) /
			singly_indirect - blknum0 * singly_indirect;
		blknum2 = (blknum - 12 - singly_indirect - doubly_indirect) -
			((blknum - 12 - singly_indirect - doubly_indirect) /
			 singly_indirect) * singly_indirect;
		if (!inode.i_block[14]) {
			bzero(blockbuf, dev->block_size);
			return 0;
		}

		err = ext2_block_read(dev, inode.i_block[14], blockbuf0);
		if (err) {
			return err;
		}
		if (!blockbuf0[blknum0]) {
			bzero(blockbuf, dev->block_size);
			return 0;
		}

		err = ext2_block_read(dev, blockbuf0[blknum0], blockbuf1);
		if (err) {
			return err;
		}
		if (!blockbuf1[blknum1]) {
			bzero(blockbuf, dev->block_size);
			return 0;
		}

		err = ext2_block_read(dev, blockbuf1[blknum1], blockbuf2);
		if (err) {
			return err;
		}
		if (!blockbuf2[blknum2]) {
			bzero(blockbuf, dev->block_size);
			return 0;
		}
		err = ext2_block_read(dev, blockbuf2[blknum2], blockbuf);
		if (err) {
			return err;
		}
	} else {
		return -EFBIG;
	}

	return 0;
}

static int ext2_file_block_write(ext2_blkdev_t *dev, u32 inum, u32 blknum,
		void *blockbuf)
{
	int err;
	ext2_inode_t inode;
	u32 blockbuf0[dev->block_size / sizeof(u32)];
	u32 blockbuf1[dev->block_size / sizeof(u32)];
	u32 blockbuf2[dev->block_size / sizeof(u32)];
	u32 blknum0, blknum1, blknum2;
	u64 singly_indirect = dev->block_size / sizeof(*inode.i_block);
	u64 doubly_indirect = singly_indirect * singly_indirect;
	u64 triply_indirect = doubly_indirect * singly_indirect;

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	if (blknum < 12) {
		if (!inode.i_block[blknum]) {
			err = ext2_file_block_allocate(dev, inum, blknum);
			if (err) {
				return err;
			}
			err = ext2_inode_read(dev, inum, &inode);
			if (err) {
				return err;
			}
		}
		err = ext2_block_write(dev, inode.i_block[blknum], blockbuf);
		if (err) {
			return err;
		}
	} else if (blknum < 12 + singly_indirect) {
		if (!inode.i_block[12]) {
			err = ext2_file_block_allocate(dev, inum, blknum);
			if (err) {
				return err;
			}
			err = ext2_inode_read(dev, inum, &inode);
			if (err) {
				return err;
			}
		}

		err = ext2_block_read(dev, inode.i_block[12], blockbuf0);
		if (err) {
			return err;
		}
		if (!blockbuf0[blknum - 12]) {
			err = ext2_file_block_allocate(dev, inum, blknum);
			if (err) {
				return err;
			}
			err = ext2_block_read(dev, inode.i_block[12], blockbuf0);
			if (err) {
				return err;
			}
		}
		err = ext2_block_write(dev, blockbuf0[blknum - 12], blockbuf);
		if (err) {
			return err;
		}
	} else if (blknum < 12 + singly_indirect + doubly_indirect) {
		blknum0 = (blknum - 12 - singly_indirect) / singly_indirect;
		blknum1 = (blknum - 12 - singly_indirect) - blknum0 * singly_indirect;
		if (!inode.i_block[13]) {
			err = ext2_file_block_allocate(dev, inum, blknum);
			if (err) {
				return err;
			}
			err = ext2_inode_read(dev, inum, &inode);
			if (err) {
				return err;
			}
		}

		err = ext2_block_read(dev, inode.i_block[13], blockbuf0);
		if (err) {
			return err;
		}
		if (!blockbuf0[blknum0]) {
			err = ext2_file_block_allocate(dev, inum, blknum);
			if (err) {
				return err;
			}
			err = ext2_block_read(dev, inode.i_block[13], blockbuf0);
			if (err) {
				return err;
			}
		}

		err = ext2_block_read(dev, blockbuf0[blknum0], blockbuf1);
		if (err) {
			return err;
		}
		if (!blockbuf1[blknum1]) {
			err = ext2_file_block_allocate(dev, inum, blknum);
			if (err) {
				return err;
			}
			err = ext2_block_read(dev, blockbuf0[blknum0], blockbuf1);
			if (err) {
				return err;
			}
		}
		err = ext2_block_read(dev, blockbuf1[blknum1], blockbuf);
		if (err) {
			return err;
		}
	} else if (blknum < 12 + singly_indirect + doubly_indirect +
			triply_indirect) {
		blknum0 = (blknum - 12 - singly_indirect - doubly_indirect) /
			doubly_indirect;
		blknum1 = (blknum - 12 - singly_indirect - doubly_indirect) /
			singly_indirect - blknum0 * singly_indirect;
		blknum2 = (blknum - 12 - singly_indirect - doubly_indirect) -
			((blknum - 12 - singly_indirect - doubly_indirect) /
			 singly_indirect) * singly_indirect;
		if (!inode.i_block[14]) {
			err = ext2_file_block_allocate(dev, inum, blknum);
			if (err) {
				return err;
			}
			err = ext2_inode_read(dev, inum, &inode);
			if (err) {
				return err;
			}
		}

		err = ext2_block_read(dev, inode.i_block[14], blockbuf0);
		if (err) {
			return err;
		}
		if (!blockbuf0[blknum0]) {
			err = ext2_file_block_allocate(dev, inum, blknum);
			if (err) {
				return err;
			}
			err = ext2_block_read(dev, inode.i_block[14], blockbuf0);
			if (err) {
				return err;
			}
		}

		err = ext2_block_read(dev, blockbuf0[blknum0], blockbuf1);
		if (err) {
			return err;
		}
		if (!blockbuf1[blknum1]) {
			err = ext2_file_block_allocate(dev, inum, blknum);
			if (err) {
				return err;
			}
			err = ext2_block_read(dev, blockbuf0[blknum0], blockbuf1);
			if (err) {
				return err;
			}
		}

		err = ext2_block_read(dev, blockbuf1[blknum1], blockbuf2);
		if (err) {
			return err;
		}
		if (!blockbuf2[blknum2]) {
			err = ext2_file_block_allocate(dev, inum, blknum);
			if (err) {
				return err;
			}
			err = ext2_block_read(dev, blockbuf1[blknum1], blockbuf2);
			if (err) {
				return err;
			}
		}
		err = ext2_block_write(dev, blockbuf2[blknum2], blockbuf);
		if (err) {
			return err;
		}
	} else {
		return -EFBIG;
	}

	return 0;
}

static ssize_t ext2_file_read(ext2_blkdev_t *dev, u32 inum, void *buf, u64 len,
		u64 offset)
{
	int err;
	ext2_inode_t inode;
	u8 blockbuf[dev->block_size];
	u64 firstblock;
	u64 lastblock;
	u64 ncopied = 0;
	u64 inblock_off, inblock_len;

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	/* check file size */
	if (EXT2_INODE_GET_I_SIZE(dev, inode) <= offset) {
		return -EIO;
	}
	if (EXT2_INODE_GET_I_SIZE(dev, inode) < offset + len) {
		len = EXT2_INODE_GET_I_SIZE(dev, inode) - offset;
	}

	firstblock = offset / dev->block_size;
	lastblock = (offset + len) / dev->block_size;

	for (u64 curblock = firstblock; curblock <= lastblock; curblock++) {
		if (curblock == firstblock) {
			inblock_off = offset - curblock * dev->block_size;
			if (dev->block_size - inblock_off > len) {
				inblock_len = len;
			} else {
				inblock_len = dev->block_size - inblock_off;
			}
		} else if (curblock == lastblock) {
			inblock_off = 0;
			inblock_len = len - ncopied;
		} else {
			inblock_off = 0;
			inblock_len = dev->block_size;
		}

		err = ext2_file_block_read(dev, inum, curblock, blockbuf);
		if (err) {
			return err;
		}

		memcpy((u8 *) buf + ncopied, blockbuf + inblock_off, inblock_len);

		ncopied += inblock_len;
	}

	return len;
}

static int ext2_symlink_read(ext2_blkdev_t *dev, u32 inum, char *pathbuf)
{
	int err;
	ext2_inode_t inode;

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFLNK) {
		return -EIO;
	}

	/* For all symlink shorter than 60 bytes long,
	 * the data is stored within the inode itself.
	 */
	size_t linklen = EXT2_INODE_GET_I_SIZE(dev, inode);
	if (linklen > 60) {
		err = ext2_file_read(dev, inum, (void *) pathbuf,
			linklen, 0);
		if (err) {
			return err;
		}
	} else {
		memcpy(pathbuf, inode.i_block, linklen);
	}
	pathbuf[linklen] = '\0';

	return 0;
}

static ssize_t ext2_file_write(ext2_blkdev_t *dev, u32 inum, void *buf, u64 len, u64 offset)
{
	int err;
	ext2_inode_t inode;
	u8 blockbuf[dev->block_size];
	u64 firstblock = offset / dev->block_size;
	u64 lastblock = (offset + len) / dev->block_size;
	u64 ncopied = 0;
	u64 inblock_off, inblock_len;

	for (u64 curblock = firstblock; curblock <= lastblock; curblock++) {
		if (curblock == firstblock) {
			inblock_off = offset - curblock * dev->block_size;
			if (dev->block_size - inblock_off > len) {
				inblock_len = len;
			} else {
				inblock_len = dev->block_size - inblock_off;
			}

			err = ext2_file_block_read(dev, inum, curblock, blockbuf);
			if (err) {
				return err;
			}
		} else if (curblock == lastblock) {
			inblock_off = 0;
			inblock_len = len - ncopied;
		} else {
			inblock_off = 0;
			inblock_len = dev->block_size;

			err = ext2_file_block_read(dev, inum, curblock, blockbuf);
			if (err) {
				return err;
			}
		}

		memcpy(blockbuf + inblock_off, (u8 *) buf + ncopied, inblock_len);

		err = ext2_file_block_write(dev, inum, curblock, blockbuf);
		if (err) {
			return err;
		}

		ncopied += inblock_len;
	}

	/* update file size */
	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}
	if (EXT2_INODE_GET_I_SIZE(dev, inode) < offset + ncopied) {
		EXT2_INODE_SET_I_SIZE(dev, inode, offset + ncopied);
	}
	err = ext2_inode_write(dev, inum, &inode);
	if (err) {
		return err;
	}

	return len;
}

static int ext2_direntry_inum_by_name(ext2_blkdev_t *dev, u32 dirinum,
		const char *name, u32 *inum)
{
	int err;
	size_t offset = 0;
	u8 direntrybuf0[sizeof(ext2_directory_entry_t) + NAME_MAX];
	ext2_directory_entry_t *direntry = (void *) &direntrybuf0;
	size_t namelen = strlen(name);

	while (1) {
		err = ext2_file_read(dev, dirinum, direntry, sizeof(*direntry),
				offset);
		if (err == -EIO) {
			return -ENOENT;
		} else if (err < 0) {
			return err;
		}

		err = ext2_file_read(dev, dirinum, direntry->name, direntry->name_len,
				offset + sizeof(*direntry));
		if (err < 0) {
			return err;
		}

		if (namelen == direntry->name_len &&
				!memcmp(name, direntry->name, namelen)) {
			*inum = direntry->inode;
			break;
		}

		offset += direntry->rec_len;
	}

	return 0;
}

static int ext2_direntry_name_by_inum(ext2_blkdev_t *dev, u32 dirinum,
		char *name, u32 inum)
{
	int err;
	size_t offset = 0;
	u8 direntrybuf0[sizeof(ext2_directory_entry_t) + NAME_MAX];
	ext2_directory_entry_t *direntry = (void *) &direntrybuf0;

	while (1) {
		err = ext2_file_read(dev, dirinum, direntry, sizeof(*direntry),
				offset);
		if (err == -EIO) {
			return -ENOENT;
		} else if (err < 0) {
			return err;
		}

		if (inum == direntry->inode) {
			err = ext2_file_read(dev, dirinum, direntry->name,
					direntry->name_len, offset + sizeof(*direntry));
			if (err < 0) {
				return err;
			}

			memcpy(name, direntry->name, direntry->name_len);
			name[direntry->name_len] = '\0';
			break;
		}

		offset += direntry->rec_len;
	}

	return 0;
}

static int ext2_direntry_create(ext2_blkdev_t *dev, u32 dirinum,
		u32 inum, const char *name, u8 filetype)
{
	int err;
	size_t offset = 0;
	u8 direntrybuf0[sizeof(ext2_directory_entry_t) + NAME_MAX];
	ext2_directory_entry_t *direntry = (void *) &direntrybuf0;
	size_t namelen = strlen(name);
	u32 tmp;

	err = ext2_direntry_inum_by_name(dev, dirinum, name, &tmp);
	if (err != -ENOENT) {
		return -EEXIST;
	}

	while (1) {
		err = ext2_file_read(dev, dirinum, direntry, sizeof(*direntry),
				offset);
		if (err == -EIO) {
			/* create new direntry */
			char blockbuf[dev->block_size];
			err = ext2_file_write(dev, dirinum, blockbuf,
					dev->block_size, offset);
			if (err < 0) {
				return err;
			}
			direntry->inode = inum;
			direntry->rec_len = dev->block_size;
			direntry->name_len = namelen;
			memcpy(direntry->name, name, namelen);
			direntry->file_type = filetype;
			err = ext2_file_write(dev, dirinum, direntry,
					sizeof(*direntry) + namelen, offset);
			if (err < 0) {
				return err;
			}
			break;
		} else if (err < 0) {
			return err;
		}

		/* if direntry is unused */
		if (!direntry->inode && direntry->rec_len >=
				sizeof(*direntry) + namelen) {
			direntry->inode = inum;
			direntry->name_len = namelen;
			memcpy(direntry->name, name, namelen);
			direntry->file_type = filetype;
			err = ext2_file_write(dev, dirinum, direntry,
					sizeof(*direntry) + namelen, offset);
			if (err < 0) {
				return err;
			}
			break;
		}

		/* try to split direntry */
		size_t left_rec_len = sizeof(*direntry) + direntry->name_len;
		left_rec_len = (offset + left_rec_len) % 4 ?
			left_rec_len + 4 - (offset + left_rec_len) % 4 :
			left_rec_len;
		size_t right_rec_len = direntry->rec_len - left_rec_len;
		if (right_rec_len >= sizeof(*direntry) + namelen) {
			direntry->rec_len = left_rec_len;
			err = ext2_file_write(dev, dirinum, direntry,
					sizeof(*direntry), offset);
			if (err < 0) {
				return err;
			}
			direntry->rec_len = right_rec_len;
			direntry->inode = inum;
			direntry->name_len = namelen;
			memcpy(direntry->name, name, namelen);
			direntry->file_type = filetype;
			err = ext2_file_write(dev, dirinum, direntry,
					sizeof(*direntry) + namelen,
					offset + left_rec_len);
			if (err < 0) {
				return err;
			}
			break;
		}

		offset += direntry->rec_len;
	}

	return 0;
}

static int ext2_direntry_delete(ext2_blkdev_t *dev, u32 dirinum, const char *name)
{
	int err;
	size_t offset = 0;
	ext2_directory_entry_t direntry0, direntry1;
	char namebuf[NAME_MAX + 1];
	while (1) {
		err = ext2_file_read(dev, dirinum, &direntry1,
				sizeof(direntry1), offset);
		if (err < 0) {
			return err;
		}
		err = ext2_file_read(dev, dirinum, namebuf,
				direntry1.name_len, offset + sizeof(direntry1));
		if (err < 0) {
			return err;
		}
		namebuf[direntry1.name_len] = '\0';
		if (!strcmp(name, namebuf)) {
			if (!(offset % dev->block_size)) {
				direntry1.inode = 0;
				err = ext2_file_write(dev, dirinum, &direntry1,
						sizeof(direntry1), offset);
				if (err < 0) {
					return err;
				}
			} else {
				u16 rec_len = direntry0.rec_len;
				direntry0.rec_len += direntry1.rec_len;
				err = ext2_file_write(dev, dirinum, &direntry0,
						sizeof(direntry0),
						offset - rec_len);
				if (err < 0) {
					return err;
				}
			}

			break;
		}

		offset += direntry1.rec_len;
		direntry0 = direntry1;
	}

	return 0;
}

static int __ext2_file_lookup(ext2_blkdev_t *dev, const char *path,
		u32 *inum, u32 relinum, bool followlink,
		u16 uid, u16 gid,
		bool r, bool w, bool x, bool s, size_t maxlinkdepth)
{
	int err;
	size_t i = 0;
	ext2_inode_t curinode;
	u32 curinum, previnum;
	char name[NAME_MAX];
	size_t pathlen = strlen(path);

	if (pathlen + 1 > PATH_MAX) {
		return -ENAMETOOLONG;
	}

	if (!pathlen) {
		return -EINVAL;
	}

	if (path[0] == '/') {
		/* absolute path */
		curinum = EXT2_ROOT_INODE;
		while (path[i] == '/') i++;
	} else {
		/* relative path */
		curinum = relinum;
	}

	err = ext2_inode_read(dev, curinum, &curinode);
	if (err) {
		return err;
	}

	if (uid && gid && s &&
		!((curinode.i_uid == uid && curinode.i_mode & EXT2_S_IXUSR) ||
		(curinode.i_gid == gid && curinode.i_mode & EXT2_S_IXGRP) ||
		curinode.i_mode & EXT2_S_IXOTH)) {
		return -EACCES;
	}

	while (path[i]) {
		size_t j = 0;
		while (path[i] != '/' && path[i]) {
			name[j] = path[i];
			i++;
			j++;
		}
		name[j] = '\0';
		while (path[i] == '/') i++;

		previnum = curinum;
		err = ext2_direntry_inum_by_name(dev, curinum, name, &curinum);
		if (err) {
			return err;
		}

		err = ext2_inode_read(dev, curinum, &curinode);
		if (err) {
			return err;
		}

		if (uid && gid && path[i] && s &&
			!((curinode.i_uid == uid && curinode.i_mode & EXT2_S_IXUSR) ||
			(curinode.i_gid == gid && curinode.i_mode & EXT2_S_IXGRP) ||
			curinode.i_mode & EXT2_S_IXOTH)) {
			return -EACCES;
		}
		if (uid && gid && !path[i] && r &&
			!((curinode.i_uid == uid && curinode.i_mode & EXT2_S_IRUSR) ||
			(curinode.i_gid == gid && curinode.i_mode & EXT2_S_IRGRP) ||
			curinode.i_mode & EXT2_S_IROTH)) {
			return -EACCES;
		}
		if (uid && gid && !path[i] && w &&
			!((curinode.i_uid == uid && curinode.i_mode & EXT2_S_IWUSR) ||
			(curinode.i_gid == gid && curinode.i_mode & EXT2_S_IWGRP) ||
			curinode.i_mode & EXT2_S_IWOTH)) {
			return -EACCES;
		}
		if (uid && gid && !path[i] && x &&
			!((curinode.i_uid == uid && curinode.i_mode & EXT2_S_IXUSR) ||
			(curinode.i_gid == gid && curinode.i_mode & EXT2_S_IXGRP) ||
			curinode.i_mode & EXT2_S_IXOTH)) {
			return -EACCES;
		}

		if ((path[i] || followlink) &&
			(curinode.i_mode & EXT2_FILE_FORMAT_MASK) ==
				EXT2_S_IFLNK) {
			char linkpath[curinode.i_size + 1];
			err = ext2_symlink_read(dev, curinum,
					linkpath);
			if (err) {
				return err;
			}

			err = __ext2_file_lookup(dev, linkpath,
					&curinum, previnum, true,
					uid, gid, 
					r, w, x, s, maxlinkdepth - 1);
			if (err) {
				return err;
			}

			err = ext2_inode_read(dev, curinum,
					&curinode);
			if (err) {
				return err;
			}
		}
	}

	*inum = curinum;

	return 0;
}

int ext2_file_lookup(ext2_blkdev_t *dev, const char *path,
		u32 *inum, u32 relinum, bool followlink,
		u16 uid, u16 gid,
		bool r, bool w, bool x, bool s)
{
	return __ext2_file_lookup(dev, path, inum, relinum, followlink,
			uid, gid, r, w, x, s, SYMLINK_MAX_DEPTH);
}

static int ext2_regular_create(ext2_blkdev_t *dev, u32 parent_inum, const char *name,
		u16 mode, u16 uid, u16 gid)
{
	int err = 0;
	u32 tmp;
	ext2_inode_t parent_inode, inode;
	size_t namelen = strlen(name);
	u32 inum;

	inode.i_mode = EXT2_S_IFREG | mode;
	inode.i_uid = uid;
	inode.i_size = 0;
	inode.i_atime = 0;
	inode.i_ctime = 0;
	inode.i_mtime = 0;
	inode.i_dtime = 0;
	inode.i_gid = gid;
	inode.i_links_count = 1;
	inode.i_blocks = 0;
	inode.i_flags = 0;
	inode.i_osd1 = 0;
	bzero(inode.i_block, 15 * sizeof(u32));
	inode.i_generation = 0;
	inode.i_file_acl = 0;
	inode.i_dir_acl = 0;
	inode.i_faddr = 0;
	bzero(inode.i_osd2, 12);

	if (namelen > NAME_MAX) {
		return -ENAMETOOLONG;
	}

	if (!namelen) {
		return -EINVAL;
	}

	err = ext2_direntry_inum_by_name(dev, parent_inum, name, &tmp);
	if (err != -ENOENT) {
		return -EEXIST;
	}

	err = ext2_inode_read(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	if ((parent_inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}

	err = ext2_inode_allocate(dev, &inum);
	if (err) {
		return err;
	}

	err = ext2_direntry_create(dev, parent_inum, inum, name, EXT2_FT_REG_FILE);
	if (err) {
		return err;
	}

	err = ext2_inode_write(dev, inum, &inode);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_hardlink_create(ext2_blkdev_t *dev, u32 parent_inum, u32 inum,
		const char *name)
{
	int err = 0;
	ext2_inode_t parent_inode, inode;
	size_t namelen = strlen(name);

	if (namelen > NAME_MAX) {
		return -ENAMETOOLONG;
	}

	if (!namelen) {
		return -EINVAL;
	}

	err = ext2_inode_read(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	if ((parent_inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}

	if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) == EXT2_S_IFDIR) {
		return -EINVAL;
	}

	err = ext2_direntry_create(dev, parent_inum, inum, name,
			EXT2_STYPE_TO_FTTYPE(inode.i_mode & EXT2_FILE_FORMAT_MASK));
	if (err) {
		return err;
	}

	inode.i_links_count++;
	err = ext2_inode_write(dev, inum, &inode);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_directory_create(ext2_blkdev_t *dev, u32 parent_inum, const char *name,
		u16 mode, u16 uid, u16 gid)
{
	int err = 0;
	u32 tmp;
	ext2_inode_t parent_inode, inode;
	size_t namelen = strlen(name);
	u32 inum;

	inode.i_mode = EXT2_S_IFDIR | mode;
	inode.i_uid = uid;
	inode.i_size = 0;
	inode.i_atime = 0;
	inode.i_ctime = 0;
	inode.i_mtime = 0;
	inode.i_dtime = 0;
	inode.i_gid = gid;
	inode.i_links_count = 1;
	inode.i_blocks = 0;
	inode.i_flags = 0;
	inode.i_osd1 = 0;
	bzero(inode.i_block, 15 * sizeof(u32));
	inode.i_generation = 0;
	inode.i_file_acl = 0;
	inode.i_dir_acl = 0;
	inode.i_faddr = 0;
	bzero(inode.i_osd2, 12);

	if (namelen > NAME_MAX) {
		return -ENAMETOOLONG;
	}

	if (!namelen) {
		return -EINVAL;
	}

	err = ext2_direntry_inum_by_name(dev, parent_inum, name, &tmp);
	if (err != -ENOENT) {
		return -EEXIST;
	}

	err = ext2_inode_read(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	if ((parent_inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}
	
	err = ext2_inode_allocate(dev, &inum);
	if (err) {
		return err;
	}

	err = ext2_direntry_create(dev, parent_inum, inum, name, EXT2_FT_DIR);
	if (err) {
		return err;
	}

	err = ext2_inode_write(dev, inum, &inode);
	if (err) {
		return err;
	}

	/* create direntry for . */
	err = ext2_direntry_create(dev, inum, inum, ".", EXT2_FT_DIR);
	if (err) {
		return err;
	}

	/* create direntry for .. */
	err = ext2_direntry_create(dev, inum, parent_inum, "..", EXT2_FT_DIR);
	if (err) {
		return err;
	}

	err = ext2_inode_read(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	parent_inode.i_links_count++;
	err = ext2_inode_write(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	inode.i_links_count++;
	err = ext2_inode_write(dev, inum, &inode);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_symlink_create(ext2_blkdev_t *dev, u32 parent_inum, const char *name,
		u16 mode, u16 uid, u16 gid, const char *symlink)
{
	int err = 0;
	u32 tmp;
	ext2_inode_t parent_inode, inode;
	size_t namelen = strlen(name);
	size_t linklen = strlen(symlink);
	u32 inum;

	inode.i_mode = EXT2_S_IFLNK | mode;
	inode.i_uid = uid;
	inode.i_size = 0;
	inode.i_atime = 0;
	inode.i_ctime = 0;
	inode.i_mtime = 0;
	inode.i_dtime = 0;
	inode.i_gid = gid;
	inode.i_links_count = 1;
	inode.i_blocks = 0;
	inode.i_flags = 0;
	inode.i_osd1 = 0;
	bzero(inode.i_block, 15 * sizeof(u32));
	inode.i_generation = 0;
	inode.i_file_acl = 0;
	inode.i_dir_acl = 0;
	inode.i_faddr = 0;
	bzero(inode.i_osd2, 12);

	if (namelen > NAME_MAX || linklen + 1 > PATH_MAX) {
		return -ENAMETOOLONG;
	}

	if (!namelen || !linklen) {
		return -EINVAL;
	}

	err = ext2_direntry_inum_by_name(dev, parent_inum, name, &tmp);
	if (err != -ENOENT) {
		return -EEXIST;
	}

	err = ext2_inode_read(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	if ((parent_inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}

	err = ext2_inode_allocate(dev, &inum);
	if (err) {
		return err;
	}

	err = ext2_direntry_create(dev, parent_inum, inum, name, EXT2_FT_SYMLINK);
	if (err) {
		return err;
	}

	err = ext2_inode_write(dev, inum, &inode);
	if (err) {
		return err;
	}

	/* For all symlink shorter than 60 bytes long,
	 * the data is stored within the inode itself.
	 */
	if (linklen > 60) {
		err = ext2_file_write(dev, inum, (void *) symlink,
			linklen, 0);
		if (err < 0) {
			return err;
		}
	} else {
		memcpy(inode.i_block, symlink, linklen);
		EXT2_INODE_SET_I_SIZE(dev, inode, linklen);
		err = ext2_inode_write(dev, inum, &inode);
		if (err) {
			return err;
		}
	}

	return 0;
}

static int ext2_regular_delete(ext2_blkdev_t *dev, u32 parent_inum, const char *name)
{
	int err = 0;
	ext2_inode_t parent_inode, inode;
	u32 inum;

	err = ext2_inode_read(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	if ((parent_inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}

	err = ext2_direntry_inum_by_name(dev, parent_inum, name, &inum);
	if (err) {
		return err;
	}

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFREG) {
		return -EINVAL;
	}

	err = ext2_direntry_delete(dev, parent_inum, name);
	if (err) {
		return err;
	}

	/* delete file */
	if (inode.i_links_count == 1 || !inode.i_links_count) {
		for (u32 i = 0;
			i < EXT2_INODE_GET_I_SIZE(dev, inode) / dev->block_size + 1;
			i++) {
			err = ext2_file_block_free(dev, inum, i);
			if (err) {
				return err;
			}
		}
		err = ext2_inode_free(dev, inum);
		if (err) {
			return err;
		}
	} else {
		inode.i_links_count--;
		err = ext2_inode_write(dev, inum, &inode);
		if (err) {
			return err;
		}
	}

	return 0;
}

static int ext2_directory_delete(ext2_blkdev_t *dev, u32 parent_inum, const char *name)
{
	int err = 0;
	size_t offset = 0;
	ext2_inode_t parent_inode, inode;
	ext2_directory_entry_t direntry;
	u32 inum;

	err = ext2_inode_read(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	if ((parent_inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}

	err = ext2_direntry_inum_by_name(dev, parent_inum, name, &inum);
	if (err) {
		return err;
	}

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	/* directory should contain only . and .. entries */
	err = ext2_file_read(dev, inum, &direntry,
			sizeof(direntry), offset);
	if (err < 0) {
		return err;
	}
	offset += direntry.rec_len;
	err = ext2_file_read(dev, inum, &direntry,
			sizeof(direntry), offset);
	if (err < 0) {
		return err;
	}
	offset += direntry.rec_len;
	err = ext2_file_read(dev, inum, &direntry,
			sizeof(direntry), offset);
	if (err != -EIO) {
		return -ENOTEMPTY;
	}
	offset = 0;

	if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -EINVAL;
	}

	err = ext2_direntry_delete(dev, parent_inum, name);
	if (err) {
		return err;
	}

	for (u32 i = 0; i < EXT2_INODE_GET_I_SIZE(dev, inode) / dev->block_size + 1;
			i++) {
		err = ext2_file_block_free(dev, inum, i);
		if (err) {
			return err;
		}
	}
	err = ext2_inode_free(dev, inum);
	if (err) {
		return err;
	}

	parent_inode.i_links_count--;
	err = ext2_inode_write(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	return 0;
}

static int ext2_symlink_delete(ext2_blkdev_t *dev, u32 parent_inum, const char *name)
{
	int err = 0;
	ext2_inode_t parent_inode, inode;
	u32 inum;

	err = ext2_inode_read(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	if ((parent_inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}

	err = ext2_direntry_inum_by_name(dev, parent_inum, name, &inum);
	if (err) {
		return err;
	}

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFLNK) {
		return -EINVAL;
	}

	err = ext2_direntry_delete(dev, parent_inum, name);
	if (err) {
		return err;
	}

	/* delete file */
	if (inode.i_links_count == 1 || !inode.i_links_count) {
		for (u32 i = 0;
			i < EXT2_INODE_GET_I_SIZE(dev, inode) / dev->block_size + 1;
			i++) {
			err = ext2_file_block_free(dev, inum, i);
			if (err) {
				return err;
			}
		}
		err = ext2_inode_free(dev, inum);
		if (err) {
			return err;
		}
	} else {
		inode.i_links_count--;
		err = ext2_inode_write(dev, inum, &inode);
		if (err) {
			return err;
		}
	}

	return 0;
}

static int ext2_file_delete(ext2_blkdev_t *dev, u32 dirinum, const char *name)
{
	int err;
	u32 inum;
	ext2_inode_t dirinode, inode;

	err = ext2_inode_read(dev, dirinum, &dirinode);
	if (err) {
		return err;
	}

	if ((dirinode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}

	err = ext2_direntry_inum_by_name(dev, dirinum, name, &inum);
	if (err) {
		return err;
	}

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) == EXT2_S_IFREG) {
		err = ext2_regular_delete(dev, dirinum, name);
		if (err) {
			return err;
		}
	} else if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) == EXT2_S_IFDIR) {
		err = ext2_directory_delete(dev, dirinum, name);
		if (err) {
			return err;
		}
	} else if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) == EXT2_S_IFLNK) {
		err = ext2_symlink_delete(dev, dirinum, name);
		if (err) {
			return err;
		}
	} else {
		return -EIO;
	}

	return 0;
}

static int ext2_directory_absolute_path(ext2_blkdev_t *dev, u32 inum, char *buf,
		u16 uid, u16 gid)
{
	int err;
	size_t i, j;
	size_t rpathlen = 0;
	char rpath[PATH_MAX];
	char namebuf[NAME_MAX + 1];
	size_t namebuflen;
	ext2_inode_t inode;
	u32 curinum = inum, parent_inum;

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}

	if (uid && gid &&
		!((inode.i_uid == uid && inode.i_mode & EXT2_S_IRUSR) ||
		(inode.i_gid == gid && inode.i_mode & EXT2_S_IRGRP) ||
		inode.i_mode & EXT2_S_IROTH)) {
		return -EACCES;
	}
	if (uid && gid &&
		!((inode.i_uid == uid && inode.i_mode & EXT2_S_IXUSR) ||
		(inode.i_gid == gid && inode.i_mode & EXT2_S_IXGRP) ||
		inode.i_mode & EXT2_S_IXOTH)) {
		return -EACCES;
	}

	while (curinum != EXT2_ROOT_INODE) {
		err = ext2_direntry_inum_by_name(dev, curinum, "..", &parent_inum);
		if (err) {
			return err;
		}

		err = ext2_inode_read(dev, parent_inum, &inode);
		if (err) {
			return err;
		}
		if (uid && gid &&
			!((inode.i_uid == uid && inode.i_mode & EXT2_S_IRUSR) ||
			(inode.i_gid == gid && inode.i_mode & EXT2_S_IRGRP) ||
			inode.i_mode & EXT2_S_IROTH)) {
			return -EACCES;
		}
		if (uid && gid &&
			!((inode.i_uid == uid && inode.i_mode & EXT2_S_IXUSR) ||
			(inode.i_gid == gid && inode.i_mode & EXT2_S_IXGRP) ||
			inode.i_mode & EXT2_S_IXOTH)) {
			return -EACCES;
		}

		err = ext2_direntry_name_by_inum(dev, parent_inum, namebuf, curinum);
		if (err) {
			return err;
		}

		namebuflen = strlen(namebuf);
		if (rpathlen + namebuflen + 1 >= PATH_MAX - 1) {
			return -ENAMETOOLONG;
		}

		memcpy(rpath + rpathlen, namebuf, namebuflen);
		rpathlen += namebuflen;
		rpath[rpathlen] = '/';
		rpathlen += 1;
		curinum = parent_inum;
	}

	if (!rpathlen) {
		buf[0] = '/';
		buf[1] = '\0';
		return 0;
	}

	i = rpathlen - 1;
	j = 0;
	while (1) {
		size_t namelen = 0;

		buf[j] = '/';
		i--;
		j++;

		while (i && rpath[i] != '/') {
			namelen++;
			i--;
		}

		if (!i) {
			namelen++;
			memcpy(buf + j, rpath, namelen);
			j += namelen;
			break;
		}

		memcpy(buf + j, rpath + i + 1, namelen);
		j += namelen;
	}
	buf[j] = '\0';

	return 0;
}

int ext2_ftruncate(ext2_blkdev_t *dev, u32 inum, size_t sz)
{
	int err;
	ext2_inode_t inode;
	size_t oldsz;

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFREG) {
		return -EINVAL;
	}

	oldsz = EXT2_INODE_GET_I_SIZE(dev, inode);
	if (sz < oldsz) {
		u32 firstblock = sz / dev->block_size + 1;
		u32 lastblock = oldsz / dev->block_size + 1;
		for (u32 i = firstblock; i < lastblock; i++) {
			err = ext2_file_block_free(dev, inum, i);
			if (err) {
				return err;
			}
		}
		err = ext2_inode_read(dev, inum, &inode);
		if (err) {
			return err;
		}
		EXT2_INODE_SET_I_SIZE(dev, inode, sz);
		err = ext2_inode_write(dev, inum, &inode);
		if (err) {
			return err;
		}
	} else if (sz > oldsz) {
		EXT2_INODE_SET_I_SIZE(dev, inode, sz);
		err = ext2_inode_write(dev, inum, &inode);
		if (err) {
			return err;
		}
	}

	return 0;
}

int ext2_readlink(ext2_blkdev_t *dev, const char *path, char *pathbuf,
		u16 uid, u16 gid, u32 relinum)
{
	int err;
	u32 inum;

	err = ext2_file_lookup(dev, path, &inum, relinum, false,
			uid, gid, false, false, false, true);
	if (err) {
		return err;
	}
	err = ext2_symlink_read(dev, inum, pathbuf);
	if (err) {
		return err;
	}
	return 0;
}

int ext2_creat(ext2_blkdev_t *dev, const char *path, u16 mode,
		u16 uid, u16 gid, u32 relinum)
{
	int err;
	u32 parent_inum;
	size_t pathlen = strlen(path);
	char pathcopy0[pathlen], pathcopy1[pathlen];
	char *pathname, *filename;
	if (mode & ~EXT2_ACCESS_RIGHTS_MASK) {
		return -EINVAL;
	}
	strcpy(pathcopy0, path);
	strcpy(pathcopy1, path);
	pathname = dirname(pathcopy0);
	filename = basename(pathcopy1);
	err = ext2_file_lookup(dev, pathname, &parent_inum, relinum, true,
			uid, gid, false, true, true, true);
	if (err) {
		return err;
	}
	err = ext2_regular_create(dev, parent_inum, filename, mode, uid, gid);
	if (err) {
		return err;
	}
	return 0;
}

int ext2_link(ext2_blkdev_t *dev, const char *oldpath, const char *newpath,
		u16 uid, u16 gid, u32 relinum)
{
	int err;
	u32 parent_inum, inum;
	size_t newpathlen = strlen(newpath);
	char newpathcopy0[newpathlen + 1], newpathcopy1[newpathlen + 1];
	char *pathname, *filename;

	strcpy(newpathcopy0, newpath);
	strcpy(newpathcopy1, newpath);
	pathname = dirname(newpathcopy0);
	filename = basename(newpathcopy1);
	err = ext2_file_lookup(dev, oldpath, &inum, relinum, false,
			uid, gid, false, false, false, true);
	if (err) {
		return err;
	}
	err = ext2_file_lookup(dev, pathname, &parent_inum, relinum, false,
			uid, gid, false, true, true, true);
	if (err) {
		return err;
	}
	err = ext2_hardlink_create(dev, parent_inum, inum, filename);
	if (err) {
		return err;
	}
	return 0;
}

int ext2_mkdir(ext2_blkdev_t *dev, const char *path, u16 mode,
		u16 uid, u16 gid, u32 relinum)
{
	int err;
	u32 parent_inum;
	size_t pathlen = strlen(path);
	char pathcopy0[pathlen + 1], pathcopy1[pathlen + 1];
	char *pathname, *filename;
	if (mode & ~EXT2_ACCESS_RIGHTS_MASK) {
		return -EINVAL;
	}
	strcpy(pathcopy0, path);
	strcpy(pathcopy1, path);
	pathname = dirname(pathcopy0);
	filename = basename(pathcopy1);
	err = ext2_file_lookup(dev, pathname, &parent_inum, relinum, true,
			uid, gid, false, true, true, true);
	if (err) {
		return err;
	}
	err = ext2_directory_create(dev, parent_inum, filename, mode, uid, gid);
	if (err) {
		return err;
	}
	return 0;
}

int ext2_symlink(ext2_blkdev_t *dev, const char *path, u16 mode,
		u16 uid, u16 gid, const char *symlink, u32 relinum)
{
	int err;
	u32 parent_inum;
	size_t pathlen = strlen(path);
	char pathcopy0[pathlen], pathcopy1[pathlen];
	char *pathname, *filename;
	if (mode & ~EXT2_ACCESS_RIGHTS_MASK) {
		return -EINVAL;
	}
	strcpy(pathcopy0, path);
	strcpy(pathcopy1, path);
	pathname = dirname(pathcopy0);
	filename = basename(pathcopy1);
	err = ext2_file_lookup(dev, pathname, &parent_inum, relinum, true,
			uid, gid, false, true, true, true);
	if (err) {
		return err;
	}
	err = ext2_symlink_create(dev, parent_inum, filename, mode, uid, gid, symlink);
	if (err) {
		return err;
	}
	return 0;
}

int ext2_unlink(ext2_blkdev_t *dev, const char *path, u16 uid, u16 gid, u32 relinum)
{
	int err;
	u32 parent_inum;
	size_t pathlen = strlen(path);
	char pathcopy0[pathlen + 1], pathcopy1[pathlen + 1];
	char *pathname, *filename;
	strcpy(pathcopy0, path);
	strcpy(pathcopy1, path);
	pathname = dirname(pathcopy0);
	filename = basename(pathcopy1);
	err = ext2_file_lookup(dev, pathname, &parent_inum, relinum, true,
			uid, gid, false, true, true, true);
	if (err) {
		return err;
	}
	err = ext2_file_delete(dev, parent_inum, filename);
	if (err) {
		return err;
	}
	return 0;
}

int ext2_rmdir(ext2_blkdev_t *dev, const char *path, u16 uid, u16 gid, u32 relinum)
{
	int err;
	u32 parent_inum;
	size_t pathlen = strlen(path);
	char pathcopy0[pathlen + 1], pathcopy1[pathlen + 1];
	char *pathname, *filename;
	strcpy(pathcopy0, path);
	strcpy(pathcopy1, path);
	pathname = dirname(pathcopy0);
	filename = basename(pathcopy1);
	err = ext2_file_lookup(dev, pathname, &parent_inum, relinum, true,
			uid, gid, false, true, true, true);
	if (err) {
		return err;
	}
	err = ext2_file_delete(dev, parent_inum, filename);
	if (err) {
		return err;
	}
	return 0;
}

int ext2_rename(ext2_blkdev_t *dev, const char *oldpath, const char *newpath,
		u16 uid, u16 gid, u32 relinum)
{
	int err;
	u32 srcinum, dstinum, parent_srcinum, parent_dstinum;
	ext2_inode_t srcinode, dstinode, parent_srcinode, parent_dstinode;
	size_t newpathlen = strlen(newpath);
	size_t oldpathlen = strlen(oldpath);
	char newpath0[newpathlen], newpath1[newpathlen];
	char oldpath0[oldpathlen], oldpath1[oldpathlen];
	char *dstfilename, *dstpathname;
	char *srcfilename, *srcpathname;

	strcpy(oldpath0, oldpath);
	strcpy(oldpath1, oldpath);
	srcpathname = dirname(oldpath0);
	srcfilename = basename(oldpath1);

	strcpy(newpath0, newpath);
	strcpy(newpath1, newpath);
	dstpathname = dirname(newpath0);
	dstfilename = basename(newpath1);

	err = ext2_file_lookup(dev, oldpath, &srcinum, relinum, false,
			uid, gid, false, false, false, true);
	if (err) {
		return err;
	}
	err = ext2_inode_read(dev, srcinum, &srcinode);
	if (err) {
		return err;
	}

	if (oldpath[oldpathlen] == '/' &&
			(srcinode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}

	err = ext2_file_lookup(dev, srcpathname, &parent_srcinum, relinum, true,
			uid, gid, false, true, true, true);
	if (err) {
		return err;
	}

	err = ext2_file_lookup(dev, newpath, &dstinum, relinum, false,
			uid, gid, false, false, false, true);
	if (err) {
		err = ext2_file_lookup(dev, dstpathname, &parent_dstinum, relinum, true,
				uid, gid, false, true, true, true);
		if (err) {
			return err;
		}
		err = ext2_inode_read(dev, parent_dstinum, &parent_dstinode);
		if (err) {
			return err;
		}

		if ((parent_dstinode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
			return -ENOTDIR;
		}

		err = ext2_direntry_create(dev, parent_dstinum, srcinum, dstfilename,
			EXT2_STYPE_TO_FTTYPE(srcinode.i_mode & EXT2_FILE_FORMAT_MASK));
		if (err) {
			return err;
		}

		err = ext2_direntry_delete(dev, parent_srcinum, srcfilename);
		if (err) {
			return err;
		}

		if ((srcinode.i_mode & EXT2_FILE_FORMAT_MASK) == EXT2_S_IFDIR) {
			err = ext2_inode_read(dev, parent_srcinum, &srcinode);
			if (err) {
				return err;
			}
			parent_srcinode.i_links_count--;
			err = ext2_inode_write(dev, parent_srcinum, &parent_srcinode);
			if (err) {
				return err;
			}

			err = ext2_direntry_delete(dev, srcinum, "..");
			if (err) {
				return err;
			}

			err = ext2_direntry_create(dev, srcinum, parent_dstinum, "..",
					EXT2_FT_DIR);
			if (err) {
				return err;
			}

			err = ext2_inode_read(dev, parent_dstinum, &parent_dstinode);
			if (err) {
				return err;
			}
			parent_dstinode.i_links_count++;
			err = ext2_inode_write(dev, parent_dstinum, &dstinode);
			if (err) {
				return err;
			}
		}

		return 0;
	}
	err = ext2_inode_read(dev, dstinum, &dstinode);
	if (err) {
		return err;
	}
	if (newpath[oldpathlen] == '/' &&
			(dstinode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}

	if ((dstinode.i_mode & EXT2_FILE_FORMAT_MASK) == EXT2_S_IFDIR) {
		u32 tmp;

		/* for permission check only */
		err = ext2_file_lookup(dev, newpath, &tmp, relinum, false,
			uid, gid, false, true, true, true);
		if (err) {
			return err;
		}

		err = ext2_direntry_inum_by_name(dev, dstinum, srcfilename, &tmp);
		if (!err) {
			int err = ext2_file_delete(dev, dstinum, srcfilename);
			if (err) {
				return err;
			}
		} else if (err != -ENOENT) {
			return err;
		}

		err = ext2_direntry_create(dev, dstinum, srcinum, srcfilename,
			EXT2_STYPE_TO_FTTYPE(srcinode.i_mode & EXT2_FILE_FORMAT_MASK));
		if (err) {
			return err;
		}

		err = ext2_direntry_delete(dev, parent_srcinum, srcfilename);
		if (err) {
			return err;
		}

		if ((srcinode.i_mode & EXT2_FILE_FORMAT_MASK) == EXT2_S_IFDIR) {
			err = ext2_inode_read(dev, parent_srcinum, &srcinode);
			if (err) {
				return err;
			}
			parent_srcinode.i_links_count--;
			err = ext2_inode_write(dev, parent_srcinum, &parent_srcinode);
			if (err) {
				return err;
			}

			err = ext2_direntry_delete(dev, srcinum, "..");
			if (err) {
				return err;
			}

			err = ext2_direntry_create(dev, srcinum, dstinum, "..",
					EXT2_FT_DIR);
			if (err) {
				return err;
			}

			err = ext2_inode_read(dev, dstinum, &dstinode);
			if (err) {
				return err;
			}
			dstinode.i_links_count++;
			err = ext2_inode_write(dev, dstinum, &dstinode);
			if (err) {
				return err;
			}
		}

		return 0;
	}

	err = ext2_file_lookup(dev, dstpathname, &parent_dstinum, relinum, true,
			uid, gid, false, true, true, true);
	if (err) {
		return err;
	}

	err = ext2_file_delete(dev, parent_dstinum, dstfilename);
	if (err) {
		return err;
	}

	err = ext2_direntry_create(dev, parent_dstinum, srcinum, dstfilename,
		EXT2_STYPE_TO_FTTYPE(srcinode.i_mode & EXT2_FILE_FORMAT_MASK));
	if (err) {
		return err;
	}

	err = ext2_direntry_delete(dev, parent_srcinum, srcfilename);
	if (err) {
		return err;
	}

	if ((srcinode.i_mode & EXT2_FILE_FORMAT_MASK) == EXT2_S_IFDIR) {
		err = ext2_inode_read(dev, parent_srcinum, &srcinode);
		if (err) {
			return err;
		}
		parent_srcinode.i_links_count--;
		err = ext2_inode_write(dev, parent_srcinum, &parent_srcinode);
		if (err) {
			return err;
		}

		err = ext2_direntry_delete(dev, srcinum, "..");
		if (err) {
			return err;
		}

		err = ext2_direntry_create(dev, srcinum, parent_dstinum, "..",
				EXT2_FT_DIR);
		if (err) {
			return err;
		}

		err = ext2_inode_read(dev, parent_dstinum, &parent_dstinode);
		if (err) {
			return err;
		}
		parent_dstinode.i_links_count++;
		err = ext2_inode_write(dev, parent_dstinum, &dstinode);
		if (err) {
			return err;
		}
	}

	return 0;
}

int ext2_truncate(ext2_blkdev_t *dev, const char *path, size_t sz,
		u16 uid, u16 gid, u32 relinum)
{
	int err;
	u32 inum;
	err = ext2_file_lookup(dev, path, &inum, relinum, true,
			uid, gid, false, true, false, true);
	if (err) {
		return err;
	}
	err = ext2_ftruncate(dev, inum, sz);
	if (err) {
		return err;
	}
	return 0;
}

int ext2_fstat(ext2_blkdev_t *dev, u32 inum, struct stat *st)
{
	int err;
	ext2_inode_t inode;

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	st->st_dev = dev->virtio_devnum;
	st->st_ino = inum;
	st->st_mode = inode.i_mode;
	st->st_nlink = inode.i_links_count;
	st->st_uid = inode.i_uid;
	st->st_gid = inode.i_gid;
	st->st_rdev = 0;
	st->st_size = EXT2_INODE_GET_I_SIZE(dev, inode);
	st->st_atime = inode.i_atime;
	st->st_mtime = inode.i_mtime;
	st->st_ctime = inode.i_ctime;
	st->st_blksize = dev->block_size;
	st->st_blocks = inode.i_blocks;

	return 0;
}

int ext2_stat(ext2_blkdev_t *dev, const char *path, struct stat *st,
		u16 uid, u16 gid, u32 relinum)
{
	int err;
	u32 inum;
	err = ext2_file_lookup(dev, path, &inum, relinum, true,
			uid, gid, false, false, false, true);
	if (err) {
		return err;
	}
	err = ext2_fstat(dev, inum, st);
	if (err) {
		return err;
	}

	return 0;
}

ssize_t ext2_regular_read(ext2_blkdev_t *dev, u32 inum, void *buf, u64 len, u64 offset)
{
	int err;
	ext2_inode_t inode;

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFREG) {
		return -EINVAL;
	}

	len = ext2_file_read(dev, inum, buf, len, offset);
	if (err < 0) {
		return err;
	}

	return len;
}

ssize_t ext2_regular_write(ext2_blkdev_t *dev, u32 inum, void *buf, u64 len, u64 offset)
{
	int err;
	ext2_inode_t inode;

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFREG) {
		return -EINVAL;
	}

	len = ext2_file_write(dev, inum, buf, len, offset);
	if (err < 0) {
		return err;
	}

	return len;
}

int ext2_fchdir(ext2_blkdev_t *dev, u32 inum, u32 *cwd)
{
	int err;
	ext2_inode_t inode;

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}

	*cwd = inum;

	return 0;
}

int ext2_chdir(ext2_blkdev_t *dev, const char *path, u16 uid, u16 gid, u32 *cwd)
{
	int err;
	u32 inum;
	
	err = ext2_file_lookup(dev, path, &inum, *cwd, true,
			uid, gid, false, false, true, true);
	if (err) {
		return err;
	}
	
	err = ext2_fchdir(dev, inum, cwd);
	if (err) {
		return err;
	}
	
	return 0;
}

int ext2_getcwd(ext2_blkdev_t *dev, u32 inum, char *buf, size_t size,
		u16 uid, u16 gid)
{
	int err;
	char path[PATH_MAX];
	size_t pathlen;

	err = ext2_directory_absolute_path(dev, inum, path, uid, gid);
	if (err) {
		return err;
	}

	pathlen = strlen(path) + 1;
	if (pathlen > size) {
		return -ERANGE;
	}

	memcpy(buf, path, pathlen);

	return 0;
}

int ext2_lstat(ext2_blkdev_t *dev, const char *path, struct stat *st,
		u16 uid, u16 gid, u32 relinum)
{
	int err;
	u32 inum;

	err = ext2_file_lookup(dev, path, &inum, relinum, false,
			uid, gid, false, false, false, true);
	if (err) {
		return err;
	}

	err = ext2_fstat(dev, inum, st);
	if (err) {
		return err;
	}

	return 0;
}

int ext2_fchmod(ext2_blkdev_t *dev, u32 inum, u16 mode)
{
	int err;
	ext2_inode_t inode;

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	inode.i_mode = (mode & EXT2_ACCESS_RIGHTS_MASK) |
		(inode.i_mode & EXT2_FILE_FORMAT_MASK);

	err = ext2_inode_write(dev, inum, &inode);
	if (err) {
		return err;
	}

	return 0;
}

int ext2_chmod(ext2_blkdev_t *dev, const char *path, u16 mode,
		u16 uid, u16 gid, u32 relinum)
{
	int err;
	u32 inum;

	err = ext2_file_lookup(dev, path, &inum, relinum, true,
			uid, gid, false, false, false, true);
	if (err) {
		return err;
	}

	err = ext2_fchmod(dev, inum, mode);
	if (err) {
		return err;
	}

	return 0;
}

int ext2_fchown(ext2_blkdev_t *dev, u32 inum, u16 uid, u16 gid)
{
	int err;
	ext2_inode_t inode;

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	inode.i_uid = uid;
	inode.i_gid = gid;

	err = ext2_inode_write(dev, inum, &inode);
	if (err) {
		return err;
	}

	return 0;
}

int ext2_chown(ext2_blkdev_t *dev, const char *path, u16 uid, u16 gid,
		u32 relinum, u16 uidval, u16 gidval)
{
	int err;
	u32 inum;

	err = ext2_file_lookup(dev, path, &inum, relinum, true,
			uid, gid, false, false, false, true);
	if (err) {
		return err;
	}

	err = ext2_fchown(dev, inum, uidval, gidval);
	if (err) {
		return err;
	}

	return 0;
}

int ext2_lchown(ext2_blkdev_t *dev, const char *path, u16 uid, u16 gid, u32 relinum,
		u16 uidval, u16 gidval)
{
	int err;
	u32 inum;

	err = ext2_file_lookup(dev, path, &inum, relinum, false,
			uid, gid, false, false, false, true);
	if (err) {
		return err;
	}

	err = ext2_fchown(dev, inum, uidval, gidval);
	if (err) {
		return err;
	}

	return 0;
}

int ext2_access(ext2_blkdev_t *dev, const char *path, int mode, u16 uid, u16 gid,
		u32 relinum)
{
	int err;
	u32 inum;
	bool r, w, x;
	r = mode & R_OK;
	w = mode & W_OK;
	x = mode & X_OK;
	err = ext2_file_lookup(dev, path, &inum, relinum, true,
			uid, gid, r, w, x, true);
	if (err) {
		return err;
	}
	return 0;
}

int ext2_utimes(ext2_blkdev_t *dev)
{
	/* not implemented */

	return 0;
}

