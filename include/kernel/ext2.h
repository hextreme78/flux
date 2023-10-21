#ifndef KERNEL_EXT2_H
#define KERNEL_EXT2_H

#include <kernel/types.h>

typedef struct ext2_superblock ext2_superblock_t;
typedef struct ext2_blockgroup_descriptor ext2_blockgroup_descriptor_t;
typedef struct ext2_inode ext2_inode_t;
typedef struct ext2_blkdev ext2_blkdev_t;
typedef struct ext2_directory_entry ext2_directory_entry_t;

#include <kernel/fs.h>
#include <kernel/mutex.h>
#include <kernel/list.h>

#define EXT2_INODE_SET_I_SIZE(devptr, inode, size) \
	({ \
		if ((devptr)->rev_level == EXT2_GOOD_OLD_REV) { \
			(inode).i_size = (size); \
		} else { \
			(inode).i_size = (u32) (size); \
			(inode).i_dir_acl = (size) >> 32; \
		} \
	})

#define EXT2_INODE_GET_I_SIZE(devptr, inode) \
	({ \
	 	u64 size; \
		if ((devptr)->rev_level == EXT2_GOOD_OLD_REV) { \
			size = (inode).i_size; \
		} else { \
			size = (u64) (inode).i_size + (((u64) (inode).i_dir_acl) << 32); \
		} \
		size; \
	})

#define EXT2_FTTYPE_TO_STYPE(filetype) \
	({ \
	 	u16 ret = 0; \
		if ((filetype) == EXT2_FT_REG_FILE) { \
			ret = EXT2_S_IFREG; \
	 	} else if ((filetype) == EXT2_FT_DIR) { \
			ret = EXT2_S_IFDIR; \
	 	} else if ((filetype) == EXT2_FT_CHRDEV) { \
			ret = EXT2_S_IFCHR; \
	 	} else if ((filetype) == EXT2_FT_BLKDEV) { \
			ret = EXT2_S_IFBLK; \
	 	} else if ((filetype) == EXT2_FT_FIFO) { \
			ret = EXT2_S_IFIFO; \
	 	} else if ((filetype) == EXT2_FT_SOCK) { \
			ret = EXT2_S_IFSOCK; \
	 	} else if ((filetype) == EXT2_FT_SYMLINK) { \
			ret = EXT2_S_IFLNK; \
		} \
		ret; \
	})

#define EXT2_STYPE_TO_FTTYPE(filetype) \
	({ \
	 	u8 ret = 0; \
		if ((filetype) == EXT2_S_IFREG) { \
			ret = EXT2_FT_REG_FILE; \
	 	} else if ((filetype) == EXT2_S_IFDIR) { \
			ret = EXT2_FT_DIR; \
	 	} else if ((filetype) == EXT2_S_IFCHR) { \
			ret = EXT2_FT_CHRDEV; \
	 	} else if ((filetype) == EXT2_S_IFBLK) { \
			ret = EXT2_FT_BLKDEV; \
	 	} else if ((filetype) == EXT2_S_IFIFO) { \
			ret = EXT2_FT_FIFO; \
	 	} else if ((filetype) == EXT2_S_IFSOCK) { \
			ret = EXT2_FT_SOCK; \
	 	} else if ((filetype) == EXT2_S_IFLNK) { \
			ret = EXT2_FT_SYMLINK; \
		} \
		ret; \
	})

#define EXT2_SUPERBLOCK_START 1024

#define EXT2_SUPER_MAGIC 0xef53

#define EXT2_BLOCKGROUP_DESCRIPTOR_TABLE_START 2048

#define EXT2_GOOD_OLD_REV 0
#define EXT2_DYNAMIC_REV  1

#define EXT2_ROOT_INODE 2

/* for good old revision */
#define EXT2_INODE_SIZE 128

/* s_state values */
#define EXT2_VALID_FS 1
#define EXT2_ERROR_FS 2

#define EXT2_S_IFMT 0xf000
/* -- file format -- */
#define EXT2_S_IFSOCK 0xc000 /* socket */
#define EXT2_S_IFLNK  0xa000 /* symbolic link */
#define EXT2_S_IFREG  0x8000 /* regular file */
#define EXT2_S_IFBLK  0x6000 /* block device */
#define EXT2_S_IFDIR  0x4000 /* directory */
#define EXT2_S_IFCHR  0x2000 /* character device */
#define EXT2_S_IFIFO  0x1000 /* fifo */
/* -- process execution user/group override -- */
#define EXT2_S_ISUID 0x0800 /* Set process User ID */
#define EXT2_S_ISGID 0x0400 /* Set process Group ID */
#define EXT2_S_ISVTX 0x0200 /* sticky bit */
/* -- access rights -- */
#define EXT2_S_IRUSR 0x0100 /* user read */
#define EXT2_S_IWUSR 0x0080 /* user write */
#define EXT2_S_IXUSR 0x0040 /* user execute */
#define EXT2_S_IRGRP 0x0020 /* group read */
#define EXT2_S_IWGRP 0x0010 /* group write */
#define EXT2_S_IXGRP 0x0008 /* group execute */
#define EXT2_S_IROTH 0x0004 /* others read */
#define EXT2_S_IWOTH 0x0002 /* others write */
#define EXT2_S_IXOTH 0x0001 /* others execute */

#define EXT2_FT_UNKNOWN  0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR      2
#define EXT2_FT_CHRDEV   3
#define EXT2_FT_BLKDEV   4
#define EXT2_FT_FIFO     5
#define EXT2_FT_SOCK     6
#define EXT2_FT_SYMLINK  7

#define EXT2_SECRM_FL        0x00000001
#define EXT2_UNRM_FL         0x00000002
#define EXT2_COMPR_FL        0x00000004
#define EXT2_SYNC_FL         0x00000008
#define EXT2_IMMUTABLE_FL    0x00000010
#define EXT2_APPEND_FL       0x00000020
#define EXT2_NODUMP_FL       0x00000040
#define EXT2_NOATIME_FL      0x00000080
#define EXT2_DIRTY_FL        0x00000100
#define EXT2_COMPRBLK_FL     0x00000200
#define EXT2_NOCOMPR_FL      0x00000400
#define EXT2_ECOMPR_FL       0x00000800
#define EXT2_BTREE_FL        0x00001000
#define EXT2_INDEX_FL        0x00001000
#define EXT2_IMAGIC_FL       0x00002000
#define EXT3_JOURNAL_DATA_FL 0x00004000
#define EXT2_RESERVED_FL     0x80000000

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
	u32 i_size; /* low 32 bits of size in rev1 */
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
	u32 i_dir_acl; /* high 32 bits of size in rev1 */
	u32 i_faddr;
	u8 i_osd2[12];
} __attribute__((packed));

struct ext2_directory_entry {
	u32 inode;
	u16 rec_len;
	u8 name_len;
	u8 file_type;
	char name[];
} __attribute__((packed));

struct ext2_blkdev {
	mutex_t lock;

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

extern ext2_blkdev_t *rootblkdev;

void ext2_init(void);
void ext2_root_mount(void);
void ext2_root_umount(void);

int ext2_nbytes_read(ext2_blkdev_t *dev, void *buf, u64 len, u64 offset);
int ext2_nbytes_write(ext2_blkdev_t *dev, void *buf, u64 len, u64 offset);
int ext2_file_lookup(ext2_blkdev_t *dev, const char *path,
		u32 *inum, u32 relinum, bool followlink,
		u16 uid, u16 gid,
		bool r, bool w, bool x, bool s);
int ext2_readlink(ext2_blkdev_t *dev, const char *path, char *pathbuf,
		u16 uid, u16 gid, u32 relinum);
int ext2_creat(ext2_blkdev_t *dev, const char *path, u16 mode,
		u16 uid, u16 gid, u32 relinum);
int ext2_link(ext2_blkdev_t *dev, const char *oldpath, const char *newpath,
		u16 uid, u16 gid, u32 relinum);
int ext2_mkdir(ext2_blkdev_t *dev, const char *path, u16 mode,
		u16 uid, u16 gid, u32 relinum);
int ext2_symlink(ext2_blkdev_t *dev, const char *path, u16 mode,
		u16 uid, u16 gid, const char *symlink, u32 relinum);
int ext2_unlink(ext2_blkdev_t *dev, const char *path, u16 uid, u16 gid, u32 relinum);
int ext2_rmdir(ext2_blkdev_t *dev, const char *path, u16 uid, u16 gid, u32 relinum);
int ext2_rename(ext2_blkdev_t *dev, const char *oldpath, const char *newpath,
		u16 uid, u16 gid, u32 relinum);
int ext2_ftruncate(ext2_blkdev_t *dev, u32 inum, size_t sz);
int ext2_truncate(ext2_blkdev_t *dev, const char *path, size_t sz,
		u16 uid, u16 gid, u32 relinum);
int ext2_fstat(ext2_blkdev_t *dev, u32 inum, struct stat *st);
int ext2_stat(ext2_blkdev_t *dev, const char *path, struct stat *st,
		u16 uid, u16 gid, u32 relinum);
ssize_t ext2_regular_read(ext2_blkdev_t *dev, u32 inum, void *buf, u64 len, u64 offset);
ssize_t ext2_regular_write(ext2_blkdev_t *dev, u32 inum, void *buf, u64 len, u64 offset);
int ext2_fchdir(ext2_blkdev_t *dev, u32 inum, u32 *cwd);
int ext2_chdir(ext2_blkdev_t *dev, const char *path, u16 uid, u16 gid, u32 *cwd);
int ext2_getcwd(ext2_blkdev_t *dev, u32 inum, char *buf, size_t size, u16 uid, u16 gid);
int ext2_lstat(ext2_blkdev_t *dev, const char *path, struct stat *st,
		u16 uid, u16 gid, u32 relinum);
int ext2_fchmod(ext2_blkdev_t *dev, u32 inum, u16 mode);
int ext2_chmod(ext2_blkdev_t *dev, const char *path, u16 mode,
		u16 uid, u16 gid, u32 relinum);
int ext2_fchown(ext2_blkdev_t *dev, u32 inum, u16 uid, u16 gid);
int ext2_chown(ext2_blkdev_t *dev, const char *path, u16 uid, u16 gid,
		u32 relinum, u16 uidval, u16 gidval);
int ext2_lchown(ext2_blkdev_t *dev, const char *path, u16 uid, u16 gid, u32 relinum,
		u16 uidval, u16 gidval);
int ext2_access(ext2_blkdev_t *dev, const char *path, int mode, u16 uid, u16 gid,
		u32 relinum);
int ext2_mkfifo(ext2_blkdev_t *dev, const char *path, u16 mode, u16 uid, u16 gid,
		u32 relinum);

#endif

