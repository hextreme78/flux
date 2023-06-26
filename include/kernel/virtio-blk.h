#ifndef KERNEL_VIRTIO_BLK_H
#define KERNEL_VIRTIO_BLK_H

#include <kernel/virtio.h>
#include <kernel/list.h>
#include <kernel/types.h>

/* features */
#define VIRTIO_BLK_F_SIZE_MAX     1
#define VIRTIO_BLK_F_SEG_MAX      2
#define VIRTIO_BLK_F_GEOMETRY     4
#define VIRTIO_BLK_F_RO           5
#define VIRTIO_BLK_F_BLK_SIZE     6
#define VIRTIO_BLK_F_FLUSH        9
#define VIRTIO_BLK_F_TOPOLOGY     10
#define VIRTIO_BLK_F_CONFIG_WCE   11
#define VIRTIO_BLK_F_DISCARD      13
#define VIRTIO_BLK_F_WRITE_ZEROES 14

/* blk queues */
#define VIRTIO_BLK_REQUESTQ 0

#define VIRTIO_BLK_SIZE 512

typedef volatile struct {
	virtio_mmio_t virtio_mmio;

	u64 capacity;
	u32 size_max;
	u32 seg_max;
	struct virtio_blk_geometry {
		u16 cylinders;
		u8 heads;
		u8 sectors;
	} geometry;
	u32 blk_size;
	struct virtio_blk_topology {
		/* # of logical blocks per physical block (log2) */
		u8 physical_block_exp;
		/* offset of first aligned logical block */
		u8 alignment_offset;
		/* suggested minimum I/O size in blocks */
		u16 min_io_size;
		/* optimal (suggested maximum) I/O size in blocks */
		u32 opt_io_size;
	} topology;
	u8 writeback;
	u8 unused0[3];
	u32 max_discard_sectors;
	u32 max_discard_seg;
	u32 discard_sector_alignment;
	u32 max_write_zeroes_sectors;
	u32 max_write_zeroes_seg;
	u8 write_zeroes_may_unmap;
	u8 unused1[3];
} /*__attribute__((packed))*/ virtio_blk_mmio_t;

typedef struct {
	u32 features;
	u64 capacity;
	virtq_t requestq;
	virtio_blk_mmio_t *base;
	list_t blk_list;
} virtio_blk_t;

/* type of request */
#define VIRTIO_BLK_T_IN           0
#define VIRTIO_BLK_T_OUT          1
#define VIRTIO_BLK_T_FLUSH        4
#define VIRTIO_BLK_T_DISCARD      11
#define VIRTIO_BLK_T_WRITE_ZEROES 13

/* type, unused0, sector fields */
#define VIRTIO_BLK_REQ_HEAD_SIZE 16

/* data field */
#define VIRTIO_BLK_REQ_DATA_SIZE VIRTIO_BLK_SIZE

/* status field */
#define VIRTIO_BLK_REQ_TAIL_SIZE 1

/* status field values */
#define VIRTIO_BLK_S_OK     0
#define VIRTIO_BLK_S_IOERR  1
#define VIRTIO_BLK_S_UNSUPP 2

typedef struct {
	/* head */
	u32 type;
	u32 unused0;
	u64 sector;
	
	/* data */
	u8 data[0];

	/* tail */
	u8 status;
} /*__attribute__((packed))*/ virtio_blk_req_t;

void virtio_blk_init(void);
void virtio_blk_dev_init(virtio_blk_mmio_t *base);
int virtio_blk_read(virtio_blk_t *dev, u64 sector, void *data);
int virtio_blk_write(virtio_blk_t *dev, u64 sector, void *data);

#endif

