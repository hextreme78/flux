#ifndef KERNEL_EXT2_H
#define KERNEL_EXT2_H

#include <kernel/types.h>

typedef struct ext2_superblock ext2_superblock_t;
typedef struct ext2_blockgroup_descriptor ext2_blockgroup_descriptor_t;
typedef struct ext2_inode ext2_inode_t;
typedef struct ext2_blkdev ext2_blkdev_t;

#include <kernel/spinlock.h>
#include <kernel/list.h>

#define EXT2_SUPERBLOCK_START 1024

#define EXT2_SUPER_MAGIC 0xef53

#define EXT2_BLOCKGROUP_DESCRIPTOR_TABLE_START 2048

#define EXT2_GOOD_OLD_REV 0
#define EXT2_DYNAMIC_REV  1

#define EXT2_ROOT_INODE 2

/* for good old revision */
#define EXT2_INODE_SIZE 128

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

	u8 unused2[248];

	/* we will not read sector 1 */
	/* u8 unused3[512]; */
} __attribute__((packed));

struct ext2_blockgroup_descriptor {
	u32 bg_block_bitmap;
	u32 bg_inode_bitmap;
	u32 bg_inode_table;
	u16 bg_free_blocks_count;
	u16 bg_free_inodes_count;
	u16 bg_used_dirs_count;
	u16 bg_pad;
	u8 bg_reserved[12];
} __attribute__((packed));

struct ext2_inode {
	u16 i_mode;
	u16 i_uid;
	u32 i_size;
	u32 i_atime;
	u32 i_ctime;
	u32 i_mtime;
	u32 i_dtime;
	u16 i_gid;
	u16 i_links_count;
	u32 i_blocks;
	u32 i_flags;
	u32 i_osd1;
	u32 i_block[15];
	u32 i_generation;
	u32 i_file_acl;
	u32 i_dir_acl;
	u32 i_faddr;
	u8 i_osd2[12];
} __attribute__((packed));

struct ext2_blkdev {
	spinlock_t lock;

	size_t virtio_devnum;

	u64 block_size;
	u32 inodes_count;
	u32 blocks_count;
	u32 blocks_per_group;
	u32 inodes_per_group;

	u32 blockgroups_count;

	u32 rev_level;

	u16 inode_size;

	list_t devlist;
};

void ext2_init(void);

int ext2_block_read(ext2_blkdev_t *dev, u64 blknum, void *buf);
int ext2_block_write(ext2_blkdev_t *dev, u64 blknum, void *buf);
int ext2_nbytes_read(ext2_blkdev_t *dev, void *buf, u64 len, u64 offset);
int ext2_nbytes_write(ext2_blkdev_t *dev, void *buf, u64 len, u64 offset);
int ext2_inode_read(ext2_blkdev_t *dev, u32 inum, ext2_inode_t *inode);
int ext2_inode_write(ext2_blkdev_t *dev, u32 inum, ext2_inode_t *inode);

#endif

