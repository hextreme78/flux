#ifndef KERNEL_VIRTIO_H
#define KERNEL_VIRTIO_H

#include <kernel/platform-virt.h>
#include <kernel/types.h>

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
	u32 magic_value;
	u32 version;
	u32 device_id;
	u32 vendor_id;
	u32 device_features;
	u32 device_features_sel;
	u32 unused0[2];
	u32 driver_features;
	u32 driver_features_sel;
	u32 unused1[2];
	u32 queue_sel;
	u32 queue_num_max;
	u32 queue_num;
	u32 unused2[2];
	u32 queue_ready;
	u32 unused3[2];
	u32 queue_notify;
	u32 unused4[3];
	u32 interrupt_status;
	u32 interrupt_ack;
	u32 unused5[2];
	u32 status;
	u32 unused6[3];
	u32 queue_desc_low;
	u32 queue_desc_high;
	u32 unused7[2];
	u32 queue_driver_low;
	u32 queue_driver_high;
	u32 unused8[2];
	u32 queue_device_low;
	u32 queue_device_high;
	u32 unused9[21];
	u32 config_generation;
} /* __attribute__((packed)) */ virtio_mmio_t;

/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT 1
/* This marks a buffer as device write-only (otherwise device read-only). */
#define VIRTQ_DESC_F_WRITE 2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT 4

typedef struct {
	/* Address (guest-physical). */
	u64 addr;
	/* Length. */
	u32 len;
	/* The flags as indicated above. */
	u16 flags;
	/* Next field if flags & NEXT */
	u16 next;
} /* __attribute__((packed)) */  virtq_desc_t;

#define VIRTQ_AVAIL_F_NO_INTERRUPT 1

typedef struct {
	u16 flags;
	u16 idx;

	u16 ring[0];

	u16 used_event; /* Only if VIRTIO_F_EVENT_IDX */
} /*__attribute__((packed))*/ virtq_avail_t;

typedef struct {
	/* Index of start of used descriptor chain. */
	u32 id;
	/*
	* The number of bytes written into the device writable portion of
	* the buffer described by the descriptor chain.
	*/
	u32 len;
} /*__attribute__((packed))*/ virtq_used_elem_t;

#define VIRTQ_USED_F_NO_NOTIFY 1

typedef struct {
	u16 flags;
	u16 idx;

	virtq_used_elem_t ring[0];

	u16 avail_event; /* Only if VIRTIO_F_EVENT_IDX */
} /*__attribute__((packed))*/ virtq_used_t;

#define VIRTQ_ERROR -1

typedef struct {
	u32 virtqsz;

	u8 *descmap;

	virtq_desc_t *desc;
	virtq_avail_t *avail;
	virtq_used_t *used;

	void *desc_unaligned;
	void *avail_unaligned;
	void *used_unaligned;
} virtq_t;

void virtio_init(void);
int virtq_init(virtio_mmio_t *base, virtq_t *virtq, u32 queue_sel);
void virtq_destroy(virtq_t *virtq);
u16 virtq_desc_alloc(virtq_t *virtq);
void virtq_desc_free(virtq_t *virtq, u16 desc);

#endif

