#include <kernel/ext2.h>
#include <kernel/alloc.h>
#include <kernel/virtio-blk.h>
#include <kernel/kprintf.h>
#include <kernel/klib.h>
#include <kernel/errno.h>

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

			blkdev = kmalloc(sizeof(*blkdev));

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

			spinlock_init(&blkdev->lock);

			list_add(&blkdev->devlist, &ext2_dev_list.devlist);
		}
	}
}

int ext2_block_read(ext2_blkdev_t *dev, u64 blknum, void *buf)
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

int ext2_block_write(ext2_blkdev_t *dev, u64 blknum, void *buf)
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

int ext2_superblock_read(ext2_blkdev_t *dev, ext2_superblock_t *superblock)
{
	int err;

	err = ext2_nbytes_read(dev, superblock, sizeof(*superblock),
			EXT2_SUPERBLOCK_START);
	if (err) {
		return err;
	}

	return 0;
}

int ext2_superblock_write(ext2_blkdev_t *dev, ext2_superblock_t *superblock)
{
	int err;

	err = ext2_nbytes_write(dev, superblock, sizeof(*superblock),
			EXT2_SUPERBLOCK_START);
	if (err) {
		return err;
	}

	return 0;
}

int ext2_blockgroup_descriptor_read(ext2_blkdev_t *dev, u32 bgnum,
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

int ext2_blockgroup_descriptor_write(ext2_blkdev_t *dev, u32 bgnum,
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

int ext2_inode_read(ext2_blkdev_t *dev, u32 inum, ext2_inode_t *inode)
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

int ext2_inode_write(ext2_blkdev_t *dev, u32 inum, ext2_inode_t *inode)
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

int ext2_inode_counter_increment(ext2_blkdev_t *dev, u32 inum)
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

int ext2_inode_counter_decrement(ext2_blkdev_t *dev, u32 inum)
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

int ext2_inode_allocate(ext2_blkdev_t *dev, u32 *inum)
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

int ext2_inode_free(ext2_blkdev_t *dev, u32 inum)
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

int ext2_block_counter_increment(ext2_blkdev_t *dev, u32 blknum)
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

int ext2_block_counter_decrement(ext2_blkdev_t *dev, u32 blknum)
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

int ext2_block_allocate(ext2_blkdev_t *dev, u32 *blknum, u32 inode_hint)
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

int ext2_block_free(ext2_blkdev_t *dev, u32 blknum)
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

int ext2_file_read(ext2_blkdev_t *dev, u32 inum, void *buf, u64 len, u64 offset)
{
	int err;
	ext2_inode_t inode;
	u8 blockbuf[dev->block_size];
	u64 firstblock = offset / dev->block_size;
	u64 lastblock = (offset + len) / dev->block_size;
	u64 ncopied = 0;
	u64 inblock_off, inblock_len;
	u32 blocknum;

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	/* check file size */
	if (EXT2_INODE_GET_I_SIZE(dev, inode) < offset + len) {
		return -EIO;
	}

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

		if (curblock < 12) {
			/* direct block pointer */
			blocknum = inode.i_block[curblock];
		} else {
			/* indirect block pointer */
			u64 singly_indirect = dev->block_size / sizeof(*inode.i_block);
			u64 doubly_indirect = singly_indirect * singly_indirect;
			u64 triply_indirect = doubly_indirect * singly_indirect;

			if (curblock < 12 + singly_indirect) {
				u64 off = 12;
				blocknum = inode.i_block[12];
				if (!blocknum) {
					goto blocknumzero;
				}
				err = ext2_block_read(dev, blocknum, blockbuf);
				if (err) {
					return err;
				}
				blocknum = ((u32 *) blockbuf)[curblock - off];
			} else if (curblock < 12 + singly_indirect + doubly_indirect) {
				u64 off = 12 + singly_indirect;
				blocknum = inode.i_block[13];
				if (!blocknum) {
					goto blocknumzero;
				}
				err = ext2_block_read(dev, blocknum, blockbuf);
				if (err) {
					return err;
				}
				blocknum = ((u32 *) blockbuf)[(curblock - off) /
					singly_indirect];
				if (!blocknum) {
					goto blocknumzero;
				}
				err = ext2_block_read(dev, blocknum, blockbuf);
				if (err) {
					return err;
				}
				off += singly_indirect * blocknum;
				blocknum = ((u32 *) blockbuf)[curblock - off];
			} else if (curblock < 12 + singly_indirect + doubly_indirect +
					triply_indirect) {
				u64 off = 12 + singly_indirect + doubly_indirect;
				blocknum = inode.i_block[14];
				if (!blocknum) {
					goto blocknumzero;
				}
				err = ext2_block_read(dev, blocknum, blockbuf);
				if (err) {
					return err;
				}
				blocknum = ((u32 *) blockbuf)[(curblock - off) /
					doubly_indirect];
				if (!blocknum) {
					goto blocknumzero;
				}
				err = ext2_block_read(dev, blocknum, blockbuf);
				if (err) {
					return err;
				}
				off += singly_indirect * blocknum;
				blocknum = ((u32 *) blockbuf)[(curblock - off) /
					singly_indirect];
				if (!blocknum) {
					goto blocknumzero;
				}
				err = ext2_block_read(dev, blocknum, blockbuf);
				if (err) {
					return err;
				}
				off += singly_indirect * blocknum;
				blocknum = ((u32 *) blockbuf)[curblock - off];
			} else {
				return -EFBIG;
			}
		}

blocknumzero:
		if (!blocknum) {
			bzero(blockbuf, dev->block_size);
		} else {
			err = ext2_block_read(dev, blocknum, blockbuf);
			if (err) {
				return err;
			}
		}

		memcpy((u8 *) buf + ncopied, blockbuf + inblock_off, inblock_len);

		ncopied += inblock_len;
	}

	return 0;
}

int ext2_symlink_read(ext2_blkdev_t *dev, u32 inum, char *pathbuf)
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

int ext2_file_write(ext2_blkdev_t *dev, u32 inum, void *buf, u64 len, u64 offset)
{
	int err;
	ext2_inode_t inode;
	u8 blockbuf[dev->block_size];
	u64 firstblock = offset / dev->block_size;
	u64 lastblock = (offset + len) / dev->block_size;
	u64 ncopied = 0;
	u64 inblock_off, inblock_len;
	u32 blocknum, prevblocknum;

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	for (u64 curblock = firstblock; curblock <= lastblock; curblock++) {
		if (curblock < 12) {
			/* direct block pointer */
			blocknum = inode.i_block[curblock];
			if (!blocknum) {
				err = ext2_block_allocate(dev, &blocknum, inum);
				if (err) {
					return err;
				}
				inode.i_block[curblock] = blocknum;
				inode.i_blocks++;
				err = ext2_inode_write(dev, inum, &inode);
				if (err) {
					return err;
				}
				bzero(blockbuf, dev->block_size);
				err = ext2_block_write(dev, blocknum, blockbuf);
				if (err) {
					return err;
				}
			}
		} else {
			/* indirect block pointer */
			u64 singly_indirect = dev->block_size / sizeof(*inode.i_block);
			u64 doubly_indirect = singly_indirect * singly_indirect;
			u64 triply_indirect = doubly_indirect * singly_indirect;

			if (curblock < 12 + singly_indirect) {
				u64 off = 12;
				blocknum = inode.i_block[12];
				if (!blocknum) {
					err = ext2_block_allocate(dev, &blocknum, inum);
					if (err) {
						return err;
					}
					inode.i_block[12] = blocknum;
					inode.i_blocks++;
					err = ext2_inode_write(dev, inum, &inode);
					if (err) {
						return err;
					}
					bzero(blockbuf, dev->block_size);
					err = ext2_block_write(dev, blocknum, blockbuf);
					if (err) {
						return err;
					}
				}
				err = ext2_block_read(dev, blocknum, blockbuf);
				if (err) {
					return err;
				}
				prevblocknum = blocknum;
				blocknum = ((u32 *) blockbuf)[curblock - off];
				if (!blocknum) {
					err = ext2_block_allocate(dev, &blocknum, inum);
					if (err) {
						return err;
					}
					inode.i_blocks++;
					err = ext2_inode_write(dev, inum, &inode);
					if (err) {
						return err;
					}
					((u32 *) blockbuf)[curblock - off] = blocknum;
					err = ext2_block_write(dev, prevblocknum,
							blockbuf);
					if (err) {
						return err;
					}
					bzero(blockbuf, dev->block_size);
					err = ext2_block_write(dev, blocknum, blockbuf);
					if (err) {
						return err;
					}
				}
			} else if (curblock < 12 + singly_indirect + doubly_indirect) {
				u64 off = 12 + singly_indirect;
				blocknum = inode.i_block[13];
				if (!blocknum) {
					err = ext2_block_allocate(dev, &blocknum, inum);
					if (err) {
						return err;
					}
					inode.i_block[13] = blocknum;
					inode.i_blocks++;
					err = ext2_inode_write(dev, inum, &inode);
					if (err) {
						return err;
					}
					bzero(blockbuf, dev->block_size);
					err = ext2_block_write(dev, blocknum, blockbuf);
					if (err) {
						return err;
					}
				}
				err = ext2_block_read(dev, blocknum, blockbuf);
				if (err) {
					return err;
				}
				prevblocknum = blocknum;
				blocknum = ((u32 *) blockbuf)[(curblock - off) /
					singly_indirect];
				if (!blocknum) {
					err = ext2_block_allocate(dev, &blocknum, inum);
					if (err) {
						return err;
					}
					inode.i_blocks++;
					err = ext2_inode_write(dev, inum, &inode);
					if (err) {
						return err;
					}
					((u32 *) blockbuf)[(curblock - off) /
						singly_indirect] = blocknum;
					err = ext2_block_write(dev, prevblocknum,
							blockbuf);
					if (err) {
						return err;
					}
					bzero(blockbuf, dev->block_size);
					err = ext2_block_write(dev, blocknum, blockbuf);
					if (err) {
						return err;
					}
				}
				err = ext2_block_read(dev, blocknum, blockbuf);
				if (err) {
					return err;
				}
				off += singly_indirect * blocknum;
				prevblocknum = blocknum;
				blocknum = ((u32 *) blockbuf)[curblock - off];
				if (!blocknum) {
					err = ext2_block_allocate(dev, &blocknum, inum);
					if (err) {
						return err;
					}
					inode.i_blocks++;
					err = ext2_inode_write(dev, inum, &inode);
					if (err) {
						return err;
					}
					((u32 *) blockbuf)[curblock - off] = blocknum;
					err = ext2_block_write(dev, prevblocknum,
							blockbuf);
					if (err) {
						return err;
					}
					bzero(blockbuf, dev->block_size);
					err = ext2_block_write(dev, blocknum, blockbuf);
					if (err) {
						return err;
					}
				}
			} else if (curblock < 12 + singly_indirect + doubly_indirect +
					triply_indirect) {
				u64 off = 12 + singly_indirect + doubly_indirect;
				blocknum = inode.i_block[14];
				if (!blocknum) {
					err = ext2_block_allocate(dev, &blocknum, inum);
					if (err) {
						return err;
					}
					inode.i_block[14] = blocknum;
					inode.i_blocks++;
					err = ext2_inode_write(dev, inum, &inode);
					if (err) {
						return err;
					}
					bzero(blockbuf, dev->block_size);
					err = ext2_block_write(dev, blocknum, blockbuf);
					if (err) {
						return err;
					}
				}
				err = ext2_block_read(dev, blocknum, blockbuf);
				if (err) {
					return err;
				}
				prevblocknum = blocknum;
				blocknum = ((u32 *) blockbuf)[(curblock - off) /
					doubly_indirect];
				if (!blocknum) {
					err = ext2_block_allocate(dev, &blocknum, inum);
					if (err) {
						return err;
					}
					inode.i_blocks++;
					err = ext2_inode_write(dev, inum, &inode);
					if (err) {
						return err;
					}
					((u32 *) blockbuf)[(curblock - off) /
						doubly_indirect] = blocknum;
					err = ext2_block_write(dev, prevblocknum,
							blockbuf);
					if (err) {
						return err;
					}
					bzero(blockbuf, dev->block_size);
					err = ext2_block_write(dev, blocknum, blockbuf);
					if (err) {
						return err;
					}
				}
				err = ext2_block_read(dev, blocknum, blockbuf);
				if (err) {
					return err;
				}
				off += singly_indirect * blocknum;
				prevblocknum = blocknum;
				blocknum = ((u32 *) blockbuf)[(curblock - off) /
					singly_indirect];
				if (!blocknum) {
					err = ext2_block_allocate(dev, &blocknum, inum);
					if (err) {
						return err;
					}
					inode.i_blocks++;
					err = ext2_inode_write(dev, inum, &inode);
					if (err) {
						return err;
					}
					((u32 *) blockbuf)[(curblock - off) /
						singly_indirect] = blocknum;
					err = ext2_block_write(dev, prevblocknum,
							blockbuf);
					if (err) {
						return err;
					}
					bzero(blockbuf, dev->block_size);
					err = ext2_block_write(dev, blocknum, blockbuf);
					if (err) {
						return err;
					}
				}
				err = ext2_block_read(dev, blocknum, blockbuf);
				if (err) {
					return err;
				}
				off += singly_indirect * blocknum;
				prevblocknum = blocknum;
				blocknum = ((u32 *) blockbuf)[curblock - off];
				if (!blocknum) {
					err = ext2_block_allocate(dev, &blocknum, inum);
					if (err) {
						return err;
					}
					inode.i_blocks++;
					err = ext2_inode_write(dev, inum, &inode);
					if (err) {
						return err;
					}
					((u32 *) blockbuf)[curblock - off] = blocknum;
					err = ext2_block_write(dev, prevblocknum,
							blockbuf);
					if (err) {
						return err;
					}
					bzero(blockbuf, dev->block_size);
					err = ext2_block_write(dev, blocknum, blockbuf);
					if (err) {
						return err;
					}
				}
			} else {
				return -EFBIG;
			}
		}

		if (curblock == firstblock) {
			inblock_off = offset - curblock * dev->block_size;
			if (dev->block_size - inblock_off > len) {
				inblock_len = len;
			} else {
				inblock_len = dev->block_size - inblock_off;
			}

			err = ext2_block_read(dev, blocknum, blockbuf);
			if (err) {
				return err;
			}
		} else if (curblock == lastblock) {
			inblock_off = 0;
			inblock_len = len - ncopied;
		} else {
			inblock_off = 0;
			inblock_len = dev->block_size;

			err = ext2_block_read(dev, blocknum, blockbuf);
			if (err) {
				return err;
			}
		}

		memcpy(blockbuf + inblock_off, (u8 *) buf + ncopied, inblock_len);

		err = ext2_block_write(dev, blocknum, blockbuf);
		if (err) {
			return err;
		}

		ncopied += inblock_len;
	}

	/* update file size */
	if (EXT2_INODE_GET_I_SIZE(dev, inode) < offset + ncopied) {
		EXT2_INODE_SET_I_SIZE(dev, inode, offset + ncopied);
	}
	err = ext2_inode_write(dev, inum, &inode);
	if (err) {
		return err;
	}

	return 0;
}

int ext2_regular_create(ext2_blkdev_t *dev, u32 parent_inum, const char *name,
		u16 mode, u16 uid, u16 gid)
{
	int err = 0;
	size_t offset = 0;
	ext2_inode_t parent_inode, inode;
	u8 direntry_buf[sizeof(ext2_directory_entry_t) + NAME_MAX];
	ext2_directory_entry_t *direntry = (void *) &direntry_buf;
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

	err = ext2_inode_read(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	if ((parent_inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}

	while (1) {
		err = ext2_file_read(dev, parent_inum, direntry, sizeof(*direntry),
				offset);
		if (err == -EIO) {
			/* create new direntry */
			err = ext2_inode_allocate(dev, &inum);
			if (err) {
				return err;
			}
			direntry->inode = inum;
			direntry->rec_len = dev->block_size;
			direntry->name_len = namelen;
			memcpy(direntry->name, name, namelen);
			direntry->file_type = EXT2_FT_REG_FILE;

			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry) + namelen, offset);
			if (err) {
				return err;
			}

			err = ext2_inode_write(dev, inum, &inode);
			if (err) {
				return err;
			}

			return 0;
		} else if (err) {
			return err;
		}

		/* if direntry is unused */
		if (!direntry->inode && direntry->rec_len >=
				sizeof(*direntry) + namelen) {
			err = ext2_inode_allocate(dev, &inum);
			if (err) {
				return err;
			}
			direntry->inode = inum;
			direntry->name_len = namelen;
			memcpy(direntry->name, name, namelen);
			direntry->file_type = EXT2_FT_REG_FILE;

			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry) + namelen, offset);
			if (err) {
				return err;
			}

			err = ext2_inode_write(dev, inum, &inode);
			if (err) {
				return err;
			}

			return 0;
		}

		/* try to split direntry */
		size_t left_rec_len = sizeof(*direntry) + direntry->name_len;
		left_rec_len = (offset + left_rec_len) % 4 ?
			left_rec_len + 4 - (offset + left_rec_len) % 4 :
			left_rec_len;
		size_t right_rec_len = direntry->rec_len - left_rec_len;
		if (right_rec_len >= sizeof(*direntry) + namelen) {
			direntry->rec_len = left_rec_len;
			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry), offset);
			if (err) {
				return err;
			}

			err = ext2_inode_allocate(dev, &inum);
			if (err) {
				return err;
			}
			direntry->rec_len = right_rec_len;
			direntry->inode = inum;
			direntry->name_len = namelen;
			memcpy(direntry->name, name, namelen);
			direntry->file_type = EXT2_FT_REG_FILE;

			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry) + namelen,
					offset + left_rec_len);
			if (err) {
				return err;
			}

			err = ext2_inode_write(dev, inum, &inode);
			if (err) {
				return err;
			}

			return 0;
		}

		offset += direntry->rec_len;
	}

	return 0;
}

int ext2_hardlink_create(ext2_blkdev_t *dev, u32 parent_inum, u32 inum, const char *name)
{
	int err = 0;
	size_t offset = 0;
	ext2_inode_t parent_inode, inode;
	u8 direntry_buf[sizeof(ext2_directory_entry_t) + NAME_MAX];
	ext2_directory_entry_t *direntry = (void *) &direntry_buf;
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
	inode.i_links_count++;

	while (1) {
		err = ext2_file_read(dev, parent_inum, direntry, sizeof(*direntry),
				offset);
		if (err == -EIO) {
			/* create new direntry */
			direntry->inode = inum;
			direntry->rec_len = dev->block_size;
			direntry->name_len = namelen;
			memcpy(direntry->name, name, namelen);
			direntry->file_type = EXT2_FT_REG_FILE;

			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry) + namelen, offset);
			if (err) {
				return err;
			}

			err = ext2_inode_write(dev, inum, &inode);
			if (err) {
				return err;
			}

			return 0;
		} else if (err) {
			return err;
		}

		/* if direntry is unused */
		if (!direntry->inode && direntry->rec_len >=
				sizeof(*direntry) + namelen) {
			direntry->inode = inum;
			direntry->name_len = namelen;
			memcpy(direntry->name, name, namelen);
			direntry->file_type = EXT2_FT_REG_FILE;

			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry) + namelen, offset);
			if (err) {
				return err;
			}

			err = ext2_inode_write(dev, inum, &inode);
			if (err) {
				return err;
			}

			return 0;
		}

		/* try to split direntry */
		size_t left_rec_len = sizeof(*direntry) + direntry->name_len;
		left_rec_len = (offset + left_rec_len) % 4 ?
			left_rec_len + 4 - (offset + left_rec_len) % 4 :
			left_rec_len;
		size_t right_rec_len = direntry->rec_len - left_rec_len;
		if (right_rec_len >= sizeof(*direntry) + namelen) {
			direntry->rec_len = left_rec_len;
			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry), offset);
			if (err) {
				return err;
			}

			direntry->rec_len = right_rec_len;
			direntry->inode = inum;
			direntry->name_len = namelen;
			memcpy(direntry->name, name, namelen);
			direntry->file_type = EXT2_FT_REG_FILE;

			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry) + namelen,
					offset + left_rec_len);
			if (err) {
				return err;
			}

			err = ext2_inode_write(dev, inum, &inode);
			if (err) {
				return err;
			}

			return 0;
		}

		offset += direntry->rec_len;
	}

	return 0;
}

int ext2_directory_create(ext2_blkdev_t *dev, u32 parent_inum, const char *name,
		u16 mode, u16 uid, u16 gid)
{
	int err = 0;
	size_t offset = 0;
	ext2_inode_t parent_inode, inode;
	u8 direntry_buf[sizeof(ext2_directory_entry_t) + NAME_MAX];
	ext2_directory_entry_t *direntry = (void *) &direntry_buf;
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

	err = ext2_inode_read(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	if ((parent_inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}

	while (1) {
		err = ext2_file_read(dev, parent_inum, direntry, sizeof(*direntry),
				offset);
		if (err == -EIO) {
			/* create new direntry */
			err = ext2_inode_allocate(dev, &inum);
			if (err) {
				return err;
			}
			direntry->inode = inum;
			direntry->rec_len = dev->block_size;
			direntry->name_len = namelen;
			memcpy(direntry->name, name, namelen);
			direntry->file_type = EXT2_FT_DIR;

			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry) + namelen, offset);
			if (err) {
				return err;
			}

			err = ext2_inode_write(dev, inum, &inode);
			if (err) {
				return err;
			}

			break;
		} else if (err) {
			return err;
		}

		/* if direntry is unused */
		if (!direntry->inode && direntry->rec_len >=
				sizeof(*direntry) + namelen) {
			err = ext2_inode_allocate(dev, &inum);
			if (err) {
				return err;
			}
			direntry->inode = inum;
			direntry->name_len = namelen;
			memcpy(direntry->name, name, namelen);
			direntry->file_type = EXT2_FT_DIR;

			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry) + namelen, offset);
			if (err) {
				return err;
			}

			err = ext2_inode_write(dev, inum, &inode);
			if (err) {
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
			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry), offset);
			if (err) {
				return err;
			}

			err = ext2_inode_allocate(dev, &inum);
			if (err) {
				return err;
			}
			direntry->rec_len = right_rec_len;
			direntry->inode = inum;
			direntry->name_len = namelen;
			memcpy(direntry->name, name, namelen);
			direntry->file_type = EXT2_FT_DIR;

			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry) + namelen,
					offset + left_rec_len);
			if (err) {
				return err;
			}

			err = ext2_inode_write(dev, inum, &inode);
			if (err) {
				return err;
			}

			break;
		}

		offset += direntry->rec_len;
	}

	char zerobuf[dev->block_size];
	bzero(zerobuf, dev->block_size);

	/* write zeroes to block */
	err = ext2_file_write(dev, inum, zerobuf,
			dev->block_size, 0);
	if (err) {
		return err;
	}

	/* create direntry for . */
	direntry->inode = inum;
	direntry->rec_len = 12;
	direntry->name_len = 1;
	direntry->file_type = EXT2_FT_DIR;
	direntry->name[0] = '.';
	err = ext2_file_write(dev, inum, direntry,
			sizeof(*direntry) + 1, 0);
	if (err) {
		return err;
	}

	/* create direntry for .. */
	direntry->inode = parent_inum;
	direntry->rec_len = dev->block_size - 12;
	direntry->name_len = 2;
	direntry->file_type = EXT2_FT_DIR;
	direntry->name[0] = '.';
	direntry->name[1] = '.';
	err = ext2_file_write(dev, inum, direntry,
			sizeof(*direntry) + 2, 12);
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

int ext2_symlink_create(ext2_blkdev_t *dev, u32 parent_inum, const char *name,
		u16 mode, u16 uid, u16 gid, const char *symlink)
{
	int err = 0;
	size_t offset = 0;
	ext2_inode_t parent_inode, inode;
	u8 direntry_buf[sizeof(ext2_directory_entry_t) + NAME_MAX];
	ext2_directory_entry_t *direntry = (void *) &direntry_buf;
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

	err = ext2_inode_read(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	if ((parent_inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}

	while (1) {
		err = ext2_file_read(dev, parent_inum, direntry, sizeof(*direntry),
				offset);
		if (err == -EIO) {
			/* create new direntry */
			err = ext2_inode_allocate(dev, &inum);
			if (err) {
				return err;
			}
			direntry->inode = inum;
			direntry->rec_len = dev->block_size;
			direntry->name_len = namelen;
			memcpy(direntry->name, name, namelen);
			direntry->file_type = EXT2_FT_SYMLINK;

			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry) + namelen, offset);
			if (err) {
				return err;
			}

			err = ext2_inode_write(dev, inum, &inode);
			if (err) {
				return err;
			}

			break;
		} else if (err) {
			return err;
		}

		/* if direntry is unused */
		if (!direntry->inode && direntry->rec_len >=
				sizeof(*direntry) + namelen) {
			err = ext2_inode_allocate(dev, &inum);
			if (err) {
				return err;
			}
			direntry->inode = inum;
			direntry->name_len = namelen;
			memcpy(direntry->name, name, namelen);
			direntry->file_type = EXT2_FT_SYMLINK;

			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry) + namelen, offset);
			if (err) {
				return err;
			}

			err = ext2_inode_write(dev, inum, &inode);
			if (err) {
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
			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry), offset);
			if (err) {
				return err;
			}

			err = ext2_inode_allocate(dev, &inum);
			if (err) {
				return err;
			}
			direntry->rec_len = right_rec_len;
			direntry->inode = inum;
			direntry->name_len = namelen;
			memcpy(direntry->name, name, namelen);
			direntry->file_type = EXT2_FT_SYMLINK;

			err = ext2_file_write(dev, parent_inum, direntry,
					sizeof(*direntry) + namelen,
					offset + left_rec_len);
			if (err) {
				return err;
			}

			err = ext2_inode_write(dev, inum, &inode);
			if (err) {
				return err;
			}

			break;
		}

		offset += direntry->rec_len;
	}

	/* For all symlink shorter than 60 bytes long,
	 * the data is stored within the inode itself.
	 */
	if (linklen > 60) {
		err = ext2_file_write(dev, inum, (void *) symlink,
			linklen, 0);
		if (err) {
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

/* get inode number via file path */
int ext2_file_lookup(ext2_blkdev_t *dev, const char *path, u32 *inum, u32 relinum,
		bool followlink)
{
	int err;
	size_t i = 0;
	ext2_inode_t curinode;
	u32 curinum;
	char name[NAME_MAX];
	u8 direntry_buf[sizeof(ext2_directory_entry_t) + NAME_MAX];
	ext2_directory_entry_t *direntry = (void *) &direntry_buf;

	if (strlen(path) + 1 > PATH_MAX) {
		return -ENAMETOOLONG;
	}

	if (path[0] == '/') {
		/* absolute path */
		curinum = EXT2_ROOT_INODE;
		i++;
	} else {
		/* relative path */
		curinum = relinum;
	}

	err = ext2_inode_read(dev, curinum, &curinode);
	if (err) {
		return err;
	}

	while (path[i]) {
		size_t offset = 0;
		size_t j = 0;
		while (path[i] != '/' && path[i] != '\0') {
			name[j] = path[i];
			i++;
			j++;
		}
		if (path[i] == '/') {
			i++;
		}

		if ((curinode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
			return -ENOTDIR;
		}

		while (1) {
			err = ext2_file_read(dev, curinum, direntry,
					sizeof(*direntry), offset);
			if (err) {
				return err;
			}

			if (direntry->name_len != j) {
				offset += direntry->rec_len;
				continue;
			}

			err = ext2_file_read(dev, curinum, direntry->name,
					direntry->name_len,
					offset + sizeof(*direntry));
			if (err) {
				return err;
			}

			if (!memcmp(name, direntry->name, direntry->name_len)) {
				u32 previnum = curinum;
				curinum = direntry->inode;
				err = ext2_inode_read(dev, curinum, &curinode);
				if (err) {
					return err;
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

					err = ext2_file_lookup(dev, linkpath,
							&curinum, previnum, true);
					if (err) {
						return err;
					}

					err = ext2_inode_read(dev, curinum,
							&curinode);
					if (err) {
						return err;
					}
				}

				break;
			}

			offset += direntry->rec_len;
		}
	}

	*inum = curinum;

	return 0;
}

int ext2_regular_delete(ext2_blkdev_t *dev, u32 parent_inum, u32 inum,
		const char *name)
{
	int err = 0;
	size_t offset = 0;
	char namebuf[NAME_MAX];
	ext2_inode_t parent_inode, inode;
	ext2_directory_entry_t direntry0, direntry1;
	u8 blockbuf0[dev->block_size],
	blockbuf1[dev->block_size], blockbuf2[dev->block_size];

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	err = ext2_inode_read(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	if ((parent_inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}
	
	if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFREG) {
		return -EINVAL;
	}

	/* delete direntry */
	while (1) {
		err = ext2_file_read(dev, parent_inum, &direntry1,
				sizeof(direntry1), offset);
		if (err) {
			return err;
		}
		err = ext2_file_read(dev, parent_inum, namebuf,
				direntry1.name_len, offset + sizeof(direntry1));
		if (err) {
			return err;
		}
		if (direntry1.inode == inum && !memcmp(name, namebuf, direntry1.name_len)) {
			if (!(offset % dev->block_size)) {
				direntry1.inode = 0;
				err = ext2_file_write(dev, parent_inum, &direntry1,
						sizeof(direntry1), offset);
				if (err) {
					return err;
				}
			} else {
				u16 rec_len = direntry0.rec_len;
				direntry0.rec_len += direntry1.rec_len;
				err = ext2_file_write(dev, parent_inum, &direntry0,
						sizeof(direntry0),
						offset - rec_len);
				if (err) {
					return err;
				}
			}

			break;
		}

		offset += direntry1.rec_len;
		direntry0 = direntry1;
	}

	/* delete file */
	if (inode.i_links_count == 1) {
		u64 ptrsperblock = dev->block_size / sizeof(*inode.i_block);
		for (u32 i = 0; i < 12; i++) {
			if (inode.i_block[i]) {
				err = ext2_block_free(dev, inode.i_block[i]);
				if (err) {
					return err;
				}
				inode.i_blocks--;
			}
			if (!inode.i_blocks) {
				goto inodefree;
			}
		}

		if (inode.i_block[12]) {
			err = ext2_block_read(dev, inode.i_block[12], blockbuf0);
			if (err) {
				return err;
			}
			for (u32 i = 0; i < ptrsperblock; i++) {
				if (((u32 *) blockbuf0)[i]) {
					err = ext2_block_free(dev,
							((u32 *) blockbuf0)[i]);
					if (err) {
						return err;
					}
					inode.i_blocks--;
				}
			}
			err = ext2_block_free(dev, inode.i_block[12]);
			if (err) {
				return err;
			}
			inode.i_blocks--;
			if (!inode.i_blocks) {
				goto inodefree;
			}
		}

		if (inode.i_block[13]) {
			err = ext2_block_read(dev, inode.i_block[13], blockbuf0);
			if (err) {
				return err;
			}
			for (u32 i = 0; i < ptrsperblock; i++) {
				if (((u32 *) blockbuf0)[i]) {
					err = ext2_block_read(dev, ((u32 *) blockbuf0)[i],
							blockbuf1);
					if (err) {
						return err;
					}
					for (u32 j = 0; j < ptrsperblock; j++) {
						if (((u32 *) blockbuf1)[j]) {
							err = ext2_block_free(dev,
								((u32 *)blockbuf1)[j]);
							if (err) {
								return err;
							}
							inode.i_blocks--;
						}
					}
					err = ext2_block_free(dev,
							((u32 *) blockbuf0)[i]);
					if (err) {
						return err;
					}
					inode.i_blocks--;
				}
			}
			err = ext2_block_free(dev, inode.i_block[13]);
			if (err) {
				return err;
			}
			inode.i_blocks--;
			if (!inode.i_blocks) {
				goto inodefree;
			}
		}

		if (inode.i_block[14]) {
			err = ext2_block_read(dev, inode.i_block[14], blockbuf0);
			if (err) {
				return err;
			}
			for (u32 i = 0; i < ptrsperblock; i++) {
				if (((u32 *) blockbuf0)[i]) {
					err = ext2_block_read(dev, ((u32 *) blockbuf0)[i],
							blockbuf1);
					if (err) {
						return err;
					}
					for (u32 j = 0; j < ptrsperblock; j++) {
						if (((u32 *) blockbuf1)[j]) {
							err = ext2_block_read(dev,
								((u32 *) blockbuf1)[j],
								blockbuf2);
							if (err) {
								return err;
							}
							for (u32 k = 0; k < ptrsperblock;
									k++) {
								if (((u32 *) blockbuf2)[k]) {
									err = ext2_block_free(dev,
										((u32 *)blockbuf2)[k]);
									if (err) {
										return err;
									}
									inode.i_blocks--;
								}
							}
							err = ext2_block_free(dev,
								((u32 *) blockbuf1)[j]);
							if (err) {
								return err;
							}
							inode.i_blocks--;
						}
					}
					err = ext2_block_free(dev,
							((u32 *) blockbuf0)[i]);
					if (err) {
						return err;
					}
					inode.i_blocks--;
				}
			}
			err = ext2_block_free(dev, inode.i_block[14]);
			if (err) {
				return err;
			}
			inode.i_blocks--;
			if (!inode.i_blocks) {
				goto inodefree;
			}
		}
inodefree:
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

int ext2_directory_delete(ext2_blkdev_t *dev, u32 parent_inum, u32 inum)
{
	int err = 0;
	size_t offset = 0;
	ext2_inode_t parent_inode, inode;
	ext2_directory_entry_t direntry0, direntry1;
	u8 blockbuf0[dev->block_size],
	blockbuf1[dev->block_size], blockbuf2[dev->block_size];

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	err = ext2_inode_read(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	/* directory should contain only . and .. entries */
	err = ext2_file_read(dev, inum, &direntry1,
			sizeof(direntry1), offset);
	if (err) {
		return err;
	}
	offset += direntry1.rec_len;
	err = ext2_file_read(dev, inum, &direntry1,
			sizeof(direntry1), offset);
	if (err) {
		return err;
	}
	offset += direntry1.rec_len;
	err = ext2_file_read(dev, inum, &direntry1,
			sizeof(direntry1), offset);
	if (err != -EIO) {
		return -ENOTEMPTY;
	}
	offset = 0;

	if ((parent_inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}
	if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -EINVAL;
	}

	/* delete direntry */
	while (1) {
		err = ext2_file_read(dev, parent_inum, &direntry1,
				sizeof(direntry1), offset);
		if (err) {
			return err;
		}
		if (direntry1.inode == inum) {
			if (!(offset % dev->block_size)) {
				direntry1.inode = 0;
				err = ext2_file_write(dev, parent_inum, &direntry1,
						sizeof(direntry1), offset);
				if (err) {
					return err;
				}
			} else {
				u16 rec_len = direntry0.rec_len;
				direntry0.rec_len += direntry1.rec_len;
				err = ext2_file_write(dev, parent_inum, &direntry0,
						sizeof(direntry0),
						offset - rec_len);
				if (err) {
					return err;
				}
			}

			break;
		}

		offset += direntry1.rec_len;
		direntry0 = direntry1;
	}

	/* delete file */
	u64 ptrsperblock = dev->block_size / sizeof(*inode.i_block);
	for (u32 i = 0; i < 12; i++) {
		if (inode.i_block[i]) {
			err = ext2_block_free(dev, inode.i_block[i]);
			if (err) {
				return err;
			}
			inode.i_blocks--;
		}
		if (!inode.i_blocks) {
			goto inodefree;
		}
	}

	if (inode.i_block[12]) {
		err = ext2_block_read(dev, inode.i_block[12], blockbuf0);
		if (err) {
			return err;
		}
		for (u32 i = 0; i < ptrsperblock; i++) {
			if (((u32 *) blockbuf0)[i]) {
				err = ext2_block_free(dev,
						((u32 *) blockbuf0)[i]);
				if (err) {
					return err;
				}
				inode.i_blocks--;
			}
		}
		err = ext2_block_free(dev, inode.i_block[12]);
		if (err) {
			return err;
		}
		inode.i_blocks--;
		if (!inode.i_blocks) {
			goto inodefree;
		}
	}

	if (inode.i_block[13]) {
		err = ext2_block_read(dev, inode.i_block[13], blockbuf0);
		if (err) {
			return err;
		}
		for (u32 i = 0; i < ptrsperblock; i++) {
			if (((u32 *) blockbuf0)[i]) {
				err = ext2_block_read(dev, ((u32 *) blockbuf0)[i],
						blockbuf1);
				if (err) {
					return err;
				}
				for (u32 j = 0; j < ptrsperblock; j++) {
					if (((u32 *) blockbuf1)[j]) {
						err = ext2_block_free(dev,
							((u32 *)blockbuf1)[j]);
						if (err) {
							return err;
						}
						inode.i_blocks--;
					}
				}
				err = ext2_block_free(dev,
						((u32 *) blockbuf0)[i]);
				if (err) {
					return err;
				}
				inode.i_blocks--;
			}
		}
		err = ext2_block_free(dev, inode.i_block[13]);
		if (err) {
			return err;
		}
		inode.i_blocks--;
		if (!inode.i_blocks) {
			goto inodefree;
		}
	}

	if (inode.i_block[14]) {
		err = ext2_block_read(dev, inode.i_block[14], blockbuf0);
		if (err) {
			return err;
		}
		for (u32 i = 0; i < ptrsperblock; i++) {
			if (((u32 *) blockbuf0)[i]) {
				err = ext2_block_read(dev, ((u32 *) blockbuf0)[i],
						blockbuf1);
				if (err) {
					return err;
				}
				for (u32 j = 0; j < ptrsperblock; j++) {
					if (((u32 *) blockbuf1)[j]) {
						err = ext2_block_read(dev,
							((u32 *) blockbuf1)[j],
							blockbuf2);
						if (err) {
							return err;
						}
						for (u32 k = 0; k < ptrsperblock;
								k++) {
							if (((u32 *) blockbuf2)[k]) {
								err = ext2_block_free(dev,
									((u32 *)blockbuf2)[k]);
								if (err) {
									return err;
								}
								inode.i_blocks--;
							}
						}
						err = ext2_block_free(dev,
							((u32 *) blockbuf1)[j]);
						if (err) {
							return err;
						}
						inode.i_blocks--;
					}
				}
				err = ext2_block_free(dev,
						((u32 *) blockbuf0)[i]);
				if (err) {
					return err;
				}
				inode.i_blocks--;
			}
		}
		err = ext2_block_free(dev, inode.i_block[14]);
		if (err) {
			return err;
		}
		inode.i_blocks--;
		if (!inode.i_blocks) {
			goto inodefree;
		}
	}
inodefree:
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

int ext2_symlink_delete(ext2_blkdev_t *dev, u32 parent_inum, u32 inum,
		const char *name)
{
	int err = 0;
	size_t offset = 0;
	ext2_inode_t parent_inode, inode;
	char namebuf[NAME_MAX];
	ext2_directory_entry_t direntry0, direntry1;
	u8 blockbuf0[dev->block_size],
	blockbuf1[dev->block_size], blockbuf2[dev->block_size];

	err = ext2_inode_read(dev, inum, &inode);
	if (err) {
		return err;
	}

	err = ext2_inode_read(dev, parent_inum, &parent_inode);
	if (err) {
		return err;
	}

	if ((parent_inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFDIR) {
		return -ENOTDIR;
	}
	if ((inode.i_mode & EXT2_FILE_FORMAT_MASK) != EXT2_S_IFLNK) {
		return -EINVAL;
	}

	/* delete direntry */
	while (1) {
		err = ext2_file_read(dev, parent_inum, &direntry1,
				sizeof(direntry1), offset);
		if (err) {
			return err;
		}
		err = ext2_file_read(dev, parent_inum, namebuf,
				direntry1.name_len, offset + sizeof(direntry1));
		if (err) {
			return err;
		}
		if (direntry1.inode == inum && !memcmp(name, namebuf, direntry1.name_len)) {
			if (!(offset % dev->block_size)) {
				direntry1.inode = 0;
				err = ext2_file_write(dev, parent_inum, &direntry1,
						sizeof(direntry1), offset);
				if (err) {
					return err;
				}
			} else {
				u16 rec_len = direntry0.rec_len;
				direntry0.rec_len += direntry1.rec_len;
				err = ext2_file_write(dev, parent_inum, &direntry0,
						sizeof(direntry0),
						offset - rec_len);
				if (err) {
					return err;
				}
			}

			break;
		}

		offset += direntry1.rec_len;
		direntry0 = direntry1;
	}

	/* delete file */
	if (inode.i_links_count == 1) {
		u64 ptrsperblock = dev->block_size / sizeof(*inode.i_block);

		if (EXT2_INODE_GET_I_SIZE(dev, inode) <= 60) {
			goto inodefree;
		}

		for (u32 i = 0; i < 12; i++) {
			if (inode.i_block[i]) {
				err = ext2_block_free(dev, inode.i_block[i]);
				if (err) {
					return err;
				}
				inode.i_blocks--;
			}
			if (!inode.i_blocks) {
				goto inodefree;
			}
		}

		if (inode.i_block[12]) {
			err = ext2_block_read(dev, inode.i_block[12], blockbuf0);
			if (err) {
				return err;
			}
			for (u32 i = 0; i < ptrsperblock; i++) {
				if (((u32 *) blockbuf0)[i]) {
					err = ext2_block_free(dev,
							((u32 *) blockbuf0)[i]);
					if (err) {
						return err;
					}
					inode.i_blocks--;
				}
			}
			err = ext2_block_free(dev, inode.i_block[12]);
			if (err) {
				return err;
			}
			inode.i_blocks--;
			if (!inode.i_blocks) {
				goto inodefree;
			}
		}

		if (inode.i_block[13]) {
			err = ext2_block_read(dev, inode.i_block[13], blockbuf0);
			if (err) {
				return err;
			}
			for (u32 i = 0; i < ptrsperblock; i++) {
				if (((u32 *) blockbuf0)[i]) {
					err = ext2_block_read(dev, ((u32 *) blockbuf0)[i],
							blockbuf1);
					if (err) {
						return err;
					}
					for (u32 j = 0; j < ptrsperblock; j++) {
						if (((u32 *) blockbuf1)[j]) {
							err = ext2_block_free(dev,
								((u32 *)blockbuf1)[j]);
							if (err) {
								return err;
							}
							inode.i_blocks--;
						}
					}
					err = ext2_block_free(dev,
							((u32 *) blockbuf0)[i]);
					if (err) {
						return err;
					}
					inode.i_blocks--;
				}
			}
			err = ext2_block_free(dev, inode.i_block[13]);
			if (err) {
				return err;
			}
			inode.i_blocks--;
			if (!inode.i_blocks) {
				goto inodefree;
			}
		}

		if (inode.i_block[14]) {
			err = ext2_block_read(dev, inode.i_block[14], blockbuf0);
			if (err) {
				return err;
			}
			for (u32 i = 0; i < ptrsperblock; i++) {
				if (((u32 *) blockbuf0)[i]) {
					err = ext2_block_read(dev, ((u32 *) blockbuf0)[i],
							blockbuf1);
					if (err) {
						return err;
					}
					for (u32 j = 0; j < ptrsperblock; j++) {
						if (((u32 *) blockbuf1)[j]) {
							err = ext2_block_read(dev,
								((u32 *) blockbuf1)[j],
								blockbuf2);
							if (err) {
								return err;
							}
							for (u32 k = 0; k < ptrsperblock;
									k++) {
								if (((u32 *) blockbuf2)[k]) {
									err = ext2_block_free(dev,
										((u32 *)blockbuf2)[k]);
									if (err) {
										return err;
									}
									inode.i_blocks--;
								}
							}
							err = ext2_block_free(dev,
								((u32 *) blockbuf1)[j]);
							if (err) {
								return err;
							}
							inode.i_blocks--;
						}
					}
					err = ext2_block_free(dev,
							((u32 *) blockbuf0)[i]);
					if (err) {
						return err;
					}
					inode.i_blocks--;
				}
			}
			err = ext2_block_free(dev, inode.i_block[14]);
			if (err) {
				return err;
			}
			inode.i_blocks--;
			if (!inode.i_blocks) {
				goto inodefree;
			}
		}
inodefree:
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

int ext2_file_rename(ext2_blkdev_t *dev, const char *oldpath, const char *newpath,
		u32 relinum)
{


	return 0;
}

int ext2_permission_check()
{

	return 0;
}

int ext2_setattr()
{

	return 0;
}

int ext2_getattr()
{

	return 0;
}


int ext2_update_time()
{

	return 0;
}

int ext2_root_mount()
{

	return 0;
}

int ext2_root_umount()
{

	return 0;
}

