#ifndef KERNEL_VIRTIO_BLK_H
#define KERNEL_VIRTIO_BLK_H

#include <kernel/virtio.h>
#include <kernel/list.h>

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

	uint64_t capacity;
	uint32_t size_max;
	uint32_t seg_max;
	struct virtio_blk_geometry {
		uint16_t cylinders;
		uint8_t heads;
		uint8_t sectors;
	} geometry;
	uint32_t blk_size;
	struct virtio_blk_topology {
		/* # of logical blocks per physical block (log2) */
		uint8_t physical_block_exp;
		/* offset of first aligned logical block */
		uint8_t alignment_offset;
		/* suggested minimum I/O size in blocks */
		uint16_t min_io_size;
		/* optimal (suggested maximum) I/O size in blocks */
		uint32_t opt_io_size;
	} topology;
	uint8_t writeback;
	uint8_t unused0[3];
	uint32_t max_discard_sectors;
	uint32_t max_discard_seg;
	uint32_t discard_sector_alignment;
	uint32_t max_write_zeroes_sectors;
	uint32_t max_write_zeroes_seg;
	uint8_t write_zeroes_may_unmap;
	uint8_t unused1[3];
} /*__attribute__((packed))*/ virtio_blk_mmio_t;

typedef struct {
	uint32_t features;
	uint64_t capacity;
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
	uint32_t type;
	uint32_t unused0;
	uint64_t sector;
	
	/* data */
	uint8_t data[0];

	/* tail */
	uint8_t status;
} /*__attribute__((packed))*/ virtio_blk_req_t;

void virtio_blk_init(void);
void virtio_blk_dev_init(virtio_blk_mmio_t *base);
int virtio_blk_read(virtio_blk_t *dev, uint64_t sector, void *data);
int virtio_blk_write(virtio_blk_t *dev, uint64_t sector, void *data);

#endif

