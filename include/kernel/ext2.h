#ifndef KERNEL_EXT2_H
#define KERNEL_EXT2_H

#include <kernel/types.h>

typedef struct ext2_superblock ext2_superblock_t;
typedef struct ext2_blkdev ext2_blkdev_t;

#include <kernel/list.h>

#define EXT2_SUPERBLOCK_SECTOR0 2
#define EXT2_SUPERBLOCK_SECTOR1 3

#define EXT2_SUPER_MAGIC 0xef53

struct ext2_superblock {
	u32 s_inodes_count;
	u32 s_blocks_count;
	u32 s_r_blocks_count;
	u32 s_free_blocks_count;
	u32 s_free_inodes_count;
	u32 s_first_data_block;
	u32 s_log_block_size;
	u32 s_log_frag_size;
	u32 s_blocks_per_group;
	u32 s_frags_per_group;
	u32 s_inodes_per_group;
	u32 s_mtime;
	u32 s_wtime;
	u16 s_mnt_count;
	u16 s_max_mnt_count;
	u16 s_magic;
	u16 s_state;
	u16 s_errors;
	u16 s_minor_rev_level;
	u32 s_lastcheck;
	u32 s_checkinterval;
	u32 s_creator_os;
	u32 s_rev_level;
	u16 s_def_resuid;
	u16 s_def_resgid;

	/* -- EXT2_DYNAMIC_REV Specific -- */
	u32 s_first_ino;
	u16 s_inode_size;
	u16 s_block_group_nr;
	u32 s_feature_compat;
	u32 s_feature_incompat;
	u32 s_feature_ro_compat;
	u8 s_uuid[16];
	u8 s_volume_name[16];
	u8 s_last_mounted[64];
	u32 s_algo_bitmap;

	/* -- Performance Hints -- */
	u8 s_prealloc_blocks;
	u8 s_prealloc_dir_blocks;
	u16 unused0;

	/* -- Journaling Support -- */
	u8 s_journal_uuid[16];
	u32 s_journal_inum;
	u32 s_journal_dev;
	u32 s_last_orphan;

	/* -- Directory Indexing Support -- */
	u8 s_hash_seed[4][4];
	u8 s_def_hash_version;
	u8 unused1[3];

	/* -- Other options -- */
	u32 s_default_mount_options;
	u32 s_first_meta_bg;
	u8 unused2[760];

} __attribute__((packed));

struct ext2_blkdev {
	size_t virtio_devnum;

	ext2_superblock_t superblock;

	u64 blksz;

	list_t devlist;
};

void ext2_init(void);

#endif

