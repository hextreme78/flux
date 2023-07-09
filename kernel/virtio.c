#include <kernel/virtio.h>
#include <kernel/virtio-blk.h>
#include <kernel/kprintf.h>
#include <kernel/alloc.h>
#include <kernel/klib.h>
#include <kernel/errno.h>
#include <kernel/wchan.h>

void virtio_init(void)
{
	virtio_blk_init();

	for (size_t i = 0; i < VIRTIO_MAX; i++) {
		virtio_mmio_t *base = VIRTIO_MMIO_BASE(i);
		if (base->magic_value != VIRTIO_MMIO_MAGIC_VALUE) {
			kprintf_s("wrong virtio magic value, ignore\n");
			continue;
		}
		if (base->version != VIRTIO_MMIO_VERSION) {
			kprintf_s("wrong virtio version, ignore\n");
			continue;
		}
		if (base->device_id == 0) {
			continue;
		}

		/* reset the device */
		base->status = 0;

		/* set the acknowledge status bit */
		base->status = VIRTIO_STATUS_ACKNOWLEDGE;

		switch (base->device_id) {
			case VIRTIO_DEVICE_ID_BLK:
				virtio_blk_dev_init(i);
				break;
			default:
				kprintf_s("unknown virtio device id\n");
		}
	}
}

void virtio_irq_handler(size_t devnum)
{
	virtio_mmio_t *base = VIRTIO_MMIO_BASE(devnum);

	switch (base->device_id) {
		case VIRTIO_DEVICE_ID_BLK:
			virtio_blk_irq_handler(devnum);
			break;
		default:
			kprintf_s("unknown virtio device id\n");
	}
}

int virtq_init(virtio_mmio_t *base, virtq_t *virtq, u32 queue_sel)
{
	/* select queue */
	base->queue_sel = queue_sel;

	/* check if queue is not already in use */
	if (base->queue_ready) {
		return -EBUSY;
	}

	/* read queue maximum size and check if value is not 0 */
	virtq->virtqsz = base->queue_num_max;
	if (!virtq->virtqsz) {
		return -ENODEV;
	}

	/* set lastusedidx to 0 */
	virtq->lastusedidx = 0;

	/* descriptor alloc table */
	virtq->descmap = kmalloc(sizeof(*virtq->descmap) * virtq->virtqsz);
	if (!virtq->descmap) {
		return -ENOMEM;
	}
	bzero(virtq->descmap, sizeof(*virtq->descmap) * virtq->virtqsz);

	/* allocate and zero the queue memory */
	virtq->desc_unaligned = kmalloc(ALIGNED_ALLOC_SZ(
				16 * virtq->virtqsz,
				VIRTIO_QUEUE_DESC_AREA_ALIGN));
	if (!virtq->desc_unaligned) {
		return -ENOMEM;
	}
	virtq->desc = ALIGNED_ALLOC_PTR(virtq->desc_unaligned,
			VIRTIO_QUEUE_DESC_AREA_ALIGN);
	bzero(virtq->desc, 16 * virtq->virtqsz);

	virtq->avail_unaligned = kmalloc(ALIGNED_ALLOC_SZ(6 + 2 * virtq->virtqsz,
				VIRTIO_QUEUE_DRIVER_AREA_ALIGN));
	if (!virtq->avail_unaligned) {
		kfree(virtq->desc_unaligned);
		return -ENOMEM;
	}
	virtq->avail = ALIGNED_ALLOC_PTR(virtq->avail_unaligned,
			VIRTIO_QUEUE_DRIVER_AREA_ALIGN);
	bzero(virtq->avail, 6 + 2 * virtq->virtqsz);

	virtq->used_unaligned = kmalloc(ALIGNED_ALLOC_SZ(6 + 8 * virtq->virtqsz,
				VIRTIO_QUEUE_DEVICE_AREA_ALIGN));
	if (!virtq->used_unaligned) {
		kfree(virtq->desc_unaligned);
		kfree(virtq->avail_unaligned);
		return -ENOMEM;
	}
	virtq->used = ALIGNED_ALLOC_PTR(virtq->used_unaligned,
			VIRTIO_QUEUE_DEVICE_AREA_ALIGN);
	bzero(virtq->used, 6 + 8 * virtq->virtqsz);

	/* notify the device about the queue size */
	base->queue_num = virtq->virtqsz;

	/* write physical addresses of the queue’s
	 * descriptor area, driver area and device area
	 */
	base->queue_desc_low = (u64) virtq->desc;
	base->queue_desc_high = (u64) virtq->desc >> 32;
	base->queue_driver_low = (u64) virtq->avail;
	base->queue_driver_high = (u64) virtq->avail >> 32;
	base->queue_device_low = (u64) virtq->used;
	base->queue_device_high = (u64) virtq->used >> 32;

	/* write 0x1 to queueready */
	base->queue_ready = 0x1;

	return 0;
}

void virtq_destroy(virtq_t *virtq)
{
	kfree(virtq->desc_unaligned);
	kfree(virtq->avail_unaligned);
	kfree(virtq->used_unaligned);
}

u16 virtq_desc_alloc(virtq_t *virtq)
{
	for (size_t desc = 0; desc < virtq->virtqsz; desc++) {
		if (!virtq->descmap[desc]) {
			virtq->descmap[desc] = 1;
			return desc;
		}
	}
	return VIRTQ_ERROR;
}

void virtq_desc_free(virtq_t *virtq, u16 desc)
{
	virtq->descmap[desc] = 0;
}

u16 virtq_desc_alloc_nofail(virtq_t *virtq, spinlock_t *lock)
{
	u16 desc;
	while ((desc = virtq_desc_alloc(virtq)) == VIRTQ_ERROR) {
		wchan_sleep(virtq->desc, lock);
	}
	return desc;
}

void virtq_desc_free_nofail(virtq_t *virtq, u16 desc)
{
	virtq_desc_free(virtq, desc);
	wchan_signal(virtq->desc);
}

