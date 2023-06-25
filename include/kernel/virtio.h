#ifndef KERNEL_VIRTIO_H
#define KERNEL_VIRTIO_H

#include <kernel/platform-virt.h>
#include <stdint.h>

#define VIRTIO_MMIO_MAX 8
#define VIRTIO_MMIO_BASE(n) ((virtio_mmio_t *) (VIRT_VIRTIO + (n) * 0x1000))

#define VIRTIO_MMIO_MAGIC_VALUE 0x74726976
#define VIRTIO_MMIO_VERSION 0x2

#define VIRTIO_DEVICE_ID_NET     1
#define VIRTIO_DEVICE_ID_BLK     2
#define VIRTIO_DEVICE_ID_CONSOLE 3
#define VIRTIO_DEVICE_ID_ENTROPY 4
#define VIRTIO_DEVICE_ID_BALLOON 5
#define VIRTIO_DEVICE_ID_SCSI    6
#define VIRTIO_DEVICE_ID_GPU     16
#define VIRTIO_DEVICE_ID_INPUT   18
#define VIRTIO_DEVICE_ID_CRYPTO  20
#define VIRTIO_DEVICE_ID_SOCKET  19

#define VIRTIO_STATUS_ACKNOWLEDGE        1
#define VIRTIO_STATUS_DRIVER             2
#define VIRTIO_STATUS_FAILED             128
#define VIRTIO_STATUS_FEATURES_OK        8
#define VIRTIO_STATUS_DRIVER_OK          4
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET 64

#define VIRTIO_F_RING_INDIRECT_DESC 28
#define VIRTIO_F_RING_EVENT_IDX     29
#define VIRTIO_F_VERSION_1          32
#define VIRTIO_F_ACCESS_PLATFORM    33
#define VIRTIO_F_RING_PACKED        34
#define VIRTIO_F_IN_ORDER           35
#define VIRTIO_F_ORDER_PLATFORM     36
#define VIRTIO_F_SR_IOV             37
#define VIRTIO_F_NOTIFICATION_DATA  38

#define VIRTIO_QUEUE_DESC_AREA_ALIGN   16
#define VIRTIO_QUEUE_DRIVER_AREA_ALIGN 2
#define VIRTIO_QUEUE_DEVICE_AREA_ALIGN 4

typedef volatile struct {
	uint32_t magic_value;
	uint32_t version;
	uint32_t device_id;
	uint32_t vendor_id;
	uint32_t device_features;
	uint32_t device_features_sel;
	uint32_t unused0[2];
	uint32_t driver_features;
	uint32_t driver_features_sel;
	uint32_t unused1[2];
	uint32_t queue_sel;
	uint32_t queue_num_max;
	uint32_t queue_num;
	uint32_t unused2[2];
	uint32_t queue_ready;
	uint32_t unused3[2];
	uint32_t queue_notify;
	uint32_t unused4[3];
	uint32_t interrupt_status;
	uint32_t interrupt_ack;
	uint32_t unused5[2];
	uint32_t status;
	uint32_t unused6[3];
	uint32_t queue_desc_low;
	uint32_t queue_desc_high;
	uint32_t unused7[2];
	uint32_t queue_driver_low;
	uint32_t queue_driver_high;
	uint32_t unused8[2];
	uint32_t queue_device_low;
	uint32_t queue_device_high;
	uint32_t unused9[21];
	uint32_t config_generation;
} /* __attribute__((packed)) */ virtio_mmio_t;

/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT 1
/* This marks a buffer as device write-only (otherwise device read-only). */
#define VIRTQ_DESC_F_WRITE 2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT 4

typedef struct {
	/* Address (guest-physical). */
	uint64_t addr;
	/* Length. */
	uint32_t len;
	/* The flags as indicated above. */
	uint16_t flags;
	/* Next field if flags & NEXT */
	uint16_t next;
} /* __attribute__((packed)) */  virtq_desc_t;

#define VIRTQ_AVAIL_F_NO_INTERRUPT 1

typedef struct {
	uint16_t flags;
	uint16_t idx;

	uint16_t ring[0];

	uint16_t used_event; /* Only if VIRTIO_F_EVENT_IDX */
} /*__attribute__((packed))*/ virtq_avail_t;

typedef struct {
	/* Index of start of used descriptor chain. */
	uint32_t id;
	/*
	* The number of bytes written into the device writable portion of
	* the buffer described by the descriptor chain.
	*/
	uint32_t len;
} /*__attribute__((packed))*/ virtq_used_elem_t;

#define VIRTQ_USED_F_NO_NOTIFY 1

typedef struct {
	uint16_t flags;
	uint16_t idx;

	virtq_used_elem_t ring[0];

	uint16_t avail_event; /* Only if VIRTIO_F_EVENT_IDX */
} /*__attribute__((packed))*/ virtq_used_t;

#define VIRTQ_ERROR -1

typedef struct {
	uint32_t virtqsz;

	uint8_t *descmap;

	virtq_desc_t *desc;
	virtq_avail_t *avail;
	virtq_used_t *used;

	void *desc_unaligned;
	void *avail_unaligned;
	void *used_unaligned;
} virtq_t;

void virtio_init(void);
int virtq_init(virtio_mmio_t *base, virtq_t *virtq, uint32_t queue_sel);
void virtq_destroy(virtq_t *virtq);
uint16_t virtq_desc_alloc(virtq_t *virtq);
void virtq_desc_free(virtq_t *virtq, uint16_t desc);

#endif

