#include <kernel/cdev-mem.h>
#include <kernel/alloc.h>
#include <kernel/klib.h>

static int cdev_mem_open(fd_t *fd, int flags, mode_t mode);
static int cdev_mem_close(fd_t *fd);
static ssize_t cdev_mem_read(fd_t *fd, void *buf, size_t n);
static ssize_t cdev_mem_write(fd_t *fd, const void *buf, size_t n);
static ssize_t cdev_mem_lseek(fd_t *fd, off_t offset, int whence);

struct device_driver cdev_mem = {
	.device_driver_name = "mem",
	.device_driver_open = cdev_mem_open,
	.device_driver_close = cdev_mem_close,
	.device_driver_read = cdev_mem_read,
	.device_driver_write = cdev_mem_write,
	.device_driver_lseek = cdev_mem_lseek
};

static int cdev_mem_open(fd_t *fd, int flags, mode_t mode)
{
	ssize_t ret = 0;
	switch (minor(fd->rdev)) {
	case CDEV_MEM_PORT:
	case CDEV_MEM_MEM:
	case CDEV_MEM_KMEM:
	case CDEV_MEM_KMSG:
		fd->roffset = fd->woffset = kmalloc(sizeof(off_t));
		if (!fd->roffset) {
			return -ENOMEM;
		}
		*fd->roffset = 0;
		break;
	case CDEV_MEM_NULL:
	case CDEV_MEM_ZERO:
	case CDEV_MEM_FULL:
	case CDEV_MEM_RANDOM:
	case CDEV_MEM_URANDOM:
		break;
	default:
		ret = -ENODEV;
	}
	return ret;
}

static int cdev_mem_close(fd_t *fd)
{
	ssize_t ret = 0;
	switch (minor(fd->rdev)) {
	case CDEV_MEM_PORT:
	case CDEV_MEM_MEM:
	case CDEV_MEM_KMEM:
	case CDEV_MEM_KMSG:
		kfree(fd->roffset);
		break;
	case CDEV_MEM_NULL:
	case CDEV_MEM_ZERO:
	case CDEV_MEM_FULL:
	case CDEV_MEM_RANDOM:
	case CDEV_MEM_URANDOM:
		break;
	default:
		ret = -ENODEV;
	}
	return ret;
}

static ssize_t kmem_read(fd_t *fd, void *buf, size_t n)
{
	void *ptr = (void *) *fd->roffset;
	if (copy_to_user(buf, ptr, n)) {
		return -EFAULT;
	}
	*fd->roffset += n;
	return n;
}

static ssize_t zero_read(fd_t *fd, void *buf, size_t n)
{
	if (bzero_user(buf, n)) {
		return -EFAULT;
	}
	return n;
}

static ssize_t random_read(fd_t *fd, void *buf, size_t n)
{
	/* not implemented */
	return 0;
}

static ssize_t urandom_read(fd_t *fd, void *buf, size_t n)
{
	/* not implemented */
	return 0;
}

static ssize_t kmsg_read(fd_t *fd, void *buf, size_t n)
{
	/* not implemented */
	return 0;
}

static ssize_t cdev_mem_read(fd_t *fd, void *buf, size_t n)
{
	ssize_t ret = 0;
	switch (minor(fd->rdev)) {
	case CDEV_MEM_PORT:
	case CDEV_MEM_MEM:
	case CDEV_MEM_KMEM:
		ret = kmem_read(fd, buf, n);
		break;
	case CDEV_MEM_NULL:
		break;
	case CDEV_MEM_ZERO:
	case CDEV_MEM_FULL:
		ret = zero_read(fd, buf, n);
		break;
	case CDEV_MEM_RANDOM:
		ret = random_read(fd, buf, n);
		break;
	case CDEV_MEM_URANDOM:
		ret = urandom_read(fd, buf, n);
		break;
	case CDEV_MEM_KMSG:
		ret = kmsg_read(fd, buf, n);
		break;
	default:
		ret = -ENODEV;
	}
	return ret;
}

static ssize_t kmem_write(fd_t *fd, const void *buf, size_t n)
{
	void *ptr = (void *) *fd->woffset;
	if (copy_from_user(ptr, buf, n)) {
		return -EFAULT;
	}
	*fd->woffset += n;
	return n;
}

static ssize_t random_write(fd_t *fd, const void *buf, size_t n)
{
	/* not implemented */
	return 0;
}

static ssize_t urandom_write(fd_t *fd, const void *buf, size_t n)
{
	/* not implemented */
	return 0;
}

static ssize_t kmsg_write(fd_t *fd, const void *buf, size_t n)
{
	/* not implemented */
	return 0;
}

static ssize_t cdev_mem_write(fd_t *fd, const void *buf, size_t n)
{
	ssize_t ret = 0;
	switch (minor(fd->rdev)) {
	case CDEV_MEM_PORT:
	case CDEV_MEM_MEM:
	case CDEV_MEM_KMEM:
		ret = kmem_write(fd, buf, n);
		break;
	case CDEV_MEM_NULL:
	case CDEV_MEM_ZERO:
		ret = n;
		break;
	case CDEV_MEM_FULL:
		ret = -ENOSPC;
		break;
	case CDEV_MEM_RANDOM:
		ret = random_write(fd, buf, n);
		break;
	case CDEV_MEM_URANDOM:
		ret = urandom_write(fd, buf, n);
		break;
	case CDEV_MEM_KMSG:
		ret = kmsg_write(fd, buf, n);
		break;
	default:
		ret = -ENODEV;
	}
	return ret;
}

static ssize_t kmem_lseek(fd_t *fd, off_t offset, int whence)
{
	if (whence == SEEK_SET) {
		*fd->roffset = offset;
	} else if (whence == SEEK_CUR) {
		*fd->roffset += offset;
	} else {
		return -EINVAL;
	}
	return *fd->roffset;
}

static ssize_t kmsg_lseek(fd_t *fd, off_t offset, int whence)
{
	/* not implemented */
	return 0;
}

static ssize_t cdev_mem_lseek(fd_t *fd, off_t offset, int whence)
{
	ssize_t ret = 0;
	switch (minor(fd->rdev)) {
	case CDEV_MEM_PORT:
	case CDEV_MEM_MEM:
	case CDEV_MEM_KMEM:
		ret = kmem_lseek(fd, offset, whence);
		break;
	case CDEV_MEM_NULL:
	case CDEV_MEM_ZERO:
	case CDEV_MEM_FULL:
		break;
	case CDEV_MEM_RANDOM:
	case CDEV_MEM_URANDOM:
		ret = -ESPIPE;
		break;
	case CDEV_MEM_KMSG:
		ret = kmsg_lseek(fd, offset, whence);
		break;
	default:
		ret = -ENODEV;
	}
	return ret;
}

