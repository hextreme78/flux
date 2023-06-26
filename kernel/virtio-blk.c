#include <kernel/virtio-blk.h>
#include <kernel/kprintf.h>
#include <kernel/alloc.h>
#include <kernel/klib.h>
#include <kernel/errno.h>

virtio_blk_t virtio_blk_list;

void virtio_blk_init(void)
{
	list_init(&virtio_blk_list.blk_list);
}

void virtio_blk_dev_init(virtio_blk_mmio_t *base)
{
	int err;
	virtio_blk_t *dev;

	dev = kmalloc(sizeof(*dev));
	if (!dev) {
		kprintf_s("virtio_blk_dev_init: no memory\n");
		return;
	}

	dev->base = base;
	list_add(&dev->blk_list, &virtio_blk_list.blk_list);

	/* set the driver status bit */
	base->virtio_mmio.status |= VIRTIO_STATUS_DRIVER;

	/* read device features */
	dev->features = base->virtio_mmio.device_features;

	/* select no features */
	base->virtio_mmio.device_features_sel = 0;

	/* set the features_ok status bit */
	base->virtio_mmio.status |= VIRTIO_STATUS_FEATURES_OK;

	/* re-read device status and ensure the features_ok bit is still set */
	if (!(base->virtio_mmio.status & VIRTIO_STATUS_FEATURES_OK)) {
		base->virtio_mmio.status |= VIRTIO_STATUS_FAILED;
		list_del(&dev->blk_list);
		kfree(dev);
		kprintf_s("virtio_blk_dev_init: features are not supported\n");
		return;
	}

	/* init requestq */
	err = virtq_init(&base->virtio_mmio, &dev->requestq,
			VIRTIO_BLK_REQUESTQ);
	if (err) {
		base->virtio_mmio.status |= VIRTIO_STATUS_FAILED;
		list_del(&dev->blk_list);
		kfree(dev);
		kprintf_s("virtio_blk_dev_init: queue init failed (%d)\n", err);
		return;
	}

	/* read number of blocks */
	dev->capacity = base->capacity;

	/* set the driver_ok status bit */	
	base->virtio_mmio.status |= VIRTIO_STATUS_DRIVER_OK;

	char blk[512] = "testtest\n\0";
	virtio_blk_write(dev, 0, blk);
}

int virtio_blk_read(virtio_blk_t *dev, u64 sector, void *data)
{
	virtio_blk_req_t *req;
	u16 desc0, desc1, desc2;

	if (sector >= dev->capacity) {
		return -EIO;
	}

	req = kmalloc(sizeof(*req));
	if (!req) {
		return -ENOMEM;
	}

	req->type = VIRTIO_BLK_T_IN;
	req->sector = sector;

	/* header descriptor */
	desc0 = virtq_desc_alloc(&dev->requestq);
	dev->requestq.desc[desc0].addr = (u64) req;
	dev->requestq.desc[desc0].len = VIRTIO_BLK_REQ_HEAD_SIZE;
	dev->requestq.desc[desc0].flags = VIRTQ_DESC_F_NEXT;

	/* data descriptor */
	desc1 = virtq_desc_alloc(&dev->requestq);
	dev->requestq.desc[desc1].addr = (u64) data;
	dev->requestq.desc[desc1].len = VIRTIO_BLK_REQ_DATA_SIZE;
	dev->requestq.desc[desc1].flags = VIRTQ_DESC_F_NEXT | VIRTQ_DESC_F_WRITE;

	/* tail descriptor */
	desc2 = virtq_desc_alloc(&dev->requestq);
	dev->requestq.desc[desc2].addr = (u64) &req->status;
	dev->requestq.desc[desc2].len = VIRTIO_BLK_REQ_TAIL_SIZE;
	dev->requestq.desc[desc2].flags = VIRTQ_DESC_F_WRITE;

	/* link all descriptors */
	dev->requestq.desc[desc0].next = desc1;
	dev->requestq.desc[desc1].next = desc2;
	dev->requestq.desc[desc2].next = 0;

	/* add request to avail ring and increment idx */
	dev->requestq.avail->ring[
		dev->requestq.avail->idx % dev->requestq.virtqsz] = desc0;
	dev->requestq.avail->idx++;

	/* notify device */
	dev->base->virtio_mmio.queue_notify = VIRTIO_BLK_REQUESTQ;

	return 0;
}

int virtio_blk_write(virtio_blk_t *dev, u64 sector, void *data)
{
	virtio_blk_req_t *req;
	u16 desc0, desc1, desc2;

	if (sector >= dev->capacity) {
		return -EIO;
	}

	req = kmalloc(sizeof(*req));
	if (!req) {
		return -ENOMEM;
	}

	req->type = VIRTIO_BLK_T_OUT;
	req->sector = sector;
	req->status = 0xff;

	/* header descriptor */
	desc0 = virtq_desc_alloc(&dev->requestq);
	dev->requestq.desc[desc0].addr = (u64) req;
	dev->requestq.desc[desc0].len = VIRTIO_BLK_REQ_HEAD_SIZE;
	dev->requestq.desc[desc0].flags = VIRTQ_DESC_F_NEXT;

	/* data descriptor */
	desc1 = virtq_desc_alloc(&dev->requestq);
	dev->requestq.desc[desc1].addr = (u64) data;
	dev->requestq.desc[desc1].len = VIRTIO_BLK_REQ_DATA_SIZE;
	dev->requestq.desc[desc1].flags = VIRTQ_DESC_F_NEXT;

	/* tail descriptor */
	desc2 = virtq_desc_alloc(&dev->requestq);
	dev->requestq.desc[desc2].addr = (u64) &req->status;
	dev->requestq.desc[desc2].len = VIRTIO_BLK_REQ_TAIL_SIZE;
	dev->requestq.desc[desc2].flags = VIRTQ_DESC_F_WRITE;

	/* link all descriptors */
	dev->requestq.desc[desc0].next = desc1;
	dev->requestq.desc[desc1].next = desc2;
	dev->requestq.desc[desc2].next = 0;

	/* add request to avail ring and increment idx */
	dev->requestq.avail->ring[
		dev->requestq.avail->idx % dev->requestq.virtqsz] = desc0;
	dev->requestq.avail->idx++;

	/* notify device */
	dev->base->virtio_mmio.queue_notify = VIRTIO_BLK_REQUESTQ;

	// wait here for op and then free descs(wchan is not implemented for now)
	//sleep();
	//virtq_desc_free(&dev->requestq, desc0);
	//virtq_desc_free(&dev->requestq, desc1);
	//virtq_desc_free(&dev->requestq, desc2);

	return 0;
}

