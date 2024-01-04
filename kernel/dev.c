#include <kernel/dev.h>
#include <kernel/kprintf.h>
#include <kernel/cdev-mem.h>

static struct device_driver_table_entry
	character_device_driver_table[DEVICE_DRIVER_TABLE_SIZE],
	block_device_driver_table[DEVICE_DRIVER_TABLE_SIZE];

void dev_init(void)
{
	int err;
	for (size_t i = 0; i < DEVICE_DRIVER_TABLE_SIZE; i++) {
		mutex_init(&character_device_driver_table[i].lock);
		mutex_init(&block_device_driver_table[i].lock);
	}
	err = character_device_driver_register(CDEV_MEM_MAJOR, &cdev_mem);
	if (err) {
		panic("can not register mem cdev");
	}
}

int character_device_driver_register(int major, device_driver_t *device_driver)
{
	int ret = 0;
	if (major < 0 || major >= DEVICE_DRIVER_TABLE_SIZE) {
		return -EINVAL;
	}
	mutex_lock(&character_device_driver_table[major].lock);
	if (character_device_driver_table[major].device_driver) {
		mutex_unlock(&character_device_driver_table[major].lock);
		return -EINVAL;
	}
	character_device_driver_table[major].device_driver = device_driver;
	if (device_driver->device_driver_init) {
		ret = device_driver->device_driver_init();
	}
	mutex_unlock(&character_device_driver_table[major].lock);
	return ret;
}

int character_device_driver_unregister(int major)
{
	device_driver_t *device_driver;
	if (major < 0 || major >= DEVICE_DRIVER_TABLE_SIZE) {
		return -EINVAL;
	}
	mutex_lock(&character_device_driver_table[major].lock);
	device_driver = character_device_driver_table[major].device_driver;
	if (device_driver->device_driver_cleanup) {
		device_driver->device_driver_cleanup();
	}
	character_device_driver_table[major].device_driver = NULL;
	mutex_unlock(&character_device_driver_table[major].lock);
	return 0;
}
	
int character_device_driver_open(fd_t *fd, int flags, mode_t mode)
{
	int ret = 0;
	device_driver_t *device_driver;
	if (major(fd->rdev) < 0 || major(fd->rdev) >= DEVICE_DRIVER_TABLE_SIZE) {
		return -EINVAL;
	}
	mutex_lock(&character_device_driver_table[major(fd->rdev)].lock);
	device_driver = character_device_driver_table[major(fd->rdev)].device_driver;
	if (device_driver->device_driver_open) {
		ret = device_driver->device_driver_open(fd, flags, mode);
	}
	mutex_unlock(&character_device_driver_table[major(fd->rdev)].lock);
	return ret;
}

int character_device_driver_close(fd_t *fd)
{
	int ret = 0;
	device_driver_t *device_driver;
	if (major(fd->rdev) < 0 || major(fd->rdev) >= DEVICE_DRIVER_TABLE_SIZE) {
		return -EINVAL;
	}
	mutex_lock(&character_device_driver_table[major(fd->rdev)].lock);
	device_driver = character_device_driver_table[major(fd->rdev)].device_driver;
	if (device_driver->device_driver_close) {
		ret = device_driver->device_driver_close(fd);
	}
	mutex_unlock(&character_device_driver_table[major(fd->rdev)].lock);
	return ret;
}

ssize_t character_device_driver_read(fd_t *fd, void *buf, size_t n)
{
	ssize_t ret = 0;
	device_driver_t *device_driver;
	if (major(fd->rdev) < 0 || major(fd->rdev) >= DEVICE_DRIVER_TABLE_SIZE) {
		return -EINVAL;
	}
	mutex_lock(&character_device_driver_table[major(fd->rdev)].lock);
	device_driver = character_device_driver_table[major(fd->rdev)].device_driver;
	if (device_driver->device_driver_read) {
		ret = device_driver->device_driver_read(fd, buf, n);
	}
	mutex_unlock(&character_device_driver_table[major(fd->rdev)].lock);
	return ret;
}

ssize_t character_device_driver_write(fd_t *fd, const void *buf, size_t n)
{
	ssize_t ret = 0;
	device_driver_t *device_driver;
	if (major(fd->rdev) < 0 || major(fd->rdev) >= DEVICE_DRIVER_TABLE_SIZE) {
		return -EINVAL;
	}
	mutex_lock(&character_device_driver_table[major(fd->rdev)].lock);
	device_driver = character_device_driver_table[major(fd->rdev)].device_driver;
	if (device_driver->device_driver_write) {
		ret = device_driver->device_driver_write(fd, buf, n);
	}
	mutex_unlock(&character_device_driver_table[major(fd->rdev)].lock);
	return ret;
}

off_t character_device_driver_lseek(fd_t *fd, off_t offset, int whence)
{
	off_t ret = 0;
	device_driver_t *device_driver;
	if (major(fd->rdev) < 0 || major(fd->rdev) >= DEVICE_DRIVER_TABLE_SIZE) {
		return -EINVAL;
	}
	mutex_lock(&character_device_driver_table[major(fd->rdev)].lock);
	device_driver = character_device_driver_table[major(fd->rdev)].device_driver;
	if (device_driver->device_driver_lseek) {
		ret = device_driver->device_driver_lseek(fd, offset, whence);
	}
	mutex_unlock(&character_device_driver_table[major(fd->rdev)].lock);
	return ret;
}

int block_device_driver_register(int major, device_driver_t *device_driver)
{
	int ret = 0;
	if (major < 0 || major >= DEVICE_DRIVER_TABLE_SIZE) {
		return -EINVAL;
	}
	mutex_lock(&block_device_driver_table[major].lock);
	if (block_device_driver_table[major].device_driver) {
		mutex_unlock(&block_device_driver_table[major].lock);
		return -EINVAL;
	}
	block_device_driver_table[major].device_driver = device_driver;
	if (device_driver->device_driver_init) {
		ret = device_driver->device_driver_init();
	}
	mutex_unlock(&block_device_driver_table[major].lock);
	return ret;
}

int block_device_driver_unregister(int major)
{
	device_driver_t *device_driver;
	if (major < 0 || major >= DEVICE_DRIVER_TABLE_SIZE) {
		return -EINVAL;
	}
	mutex_lock(&block_device_driver_table[major].lock);
	device_driver = block_device_driver_table[major].device_driver;
	if (device_driver->device_driver_cleanup) {
		device_driver->device_driver_cleanup();
	}
	block_device_driver_table[major].device_driver = NULL;
	mutex_unlock(&block_device_driver_table[major].lock);
	return 0;
}

int block_device_driver_open(fd_t *fd, int flags, mode_t mode)
{
	int ret = 0;
	device_driver_t *device_driver;
	if (major(fd->rdev) < 0 || major(fd->rdev) >= DEVICE_DRIVER_TABLE_SIZE) {
		return -EINVAL;
	}
	mutex_lock(&block_device_driver_table[major(fd->rdev)].lock);
	device_driver = block_device_driver_table[major(fd->rdev)].device_driver;
	if (device_driver->device_driver_open) {
		ret = device_driver->device_driver_open(fd, flags, mode);
	}
	mutex_unlock(&block_device_driver_table[major(fd->rdev)].lock);
	return ret;
}

int block_device_driver_close(fd_t *fd)
{
	int ret = 0;
	device_driver_t *device_driver;
	if (major(fd->rdev) < 0 || major(fd->rdev) >= DEVICE_DRIVER_TABLE_SIZE) {
		return -EINVAL;
	}
	mutex_lock(&block_device_driver_table[major(fd->rdev)].lock);
	device_driver = block_device_driver_table[major(fd->rdev)].device_driver;
	if (device_driver->device_driver_close) {
		ret = device_driver->device_driver_close(fd);
	}
	mutex_unlock(&block_device_driver_table[major(fd->rdev)].lock);
	return ret;
}

ssize_t block_device_driver_read(fd_t *fd, void *buf, size_t n)
{
	ssize_t ret = 0;
	device_driver_t *device_driver;
	if (major(fd->rdev) < 0 || major(fd->rdev) >= DEVICE_DRIVER_TABLE_SIZE) {
		return -EINVAL;
	}
	mutex_lock(&block_device_driver_table[major(fd->rdev)].lock);
	device_driver = block_device_driver_table[major(fd->rdev)].device_driver;
	if (device_driver->device_driver_read) {
		ret = device_driver->device_driver_read(fd, buf, n);
	}
	mutex_unlock(&block_device_driver_table[major(fd->rdev)].lock);
	return ret;
}

ssize_t block_device_driver_write(fd_t *fd, const void *buf, size_t n)
{
	ssize_t ret = 0;
	device_driver_t *device_driver;
	if (major(fd->rdev) < 0 || major(fd->rdev) >= DEVICE_DRIVER_TABLE_SIZE) {
		return -EINVAL;
	}
	mutex_lock(&block_device_driver_table[major(fd->rdev)].lock);
	device_driver = block_device_driver_table[major(fd->rdev)].device_driver;
	if (device_driver->device_driver_write) {
		ret = device_driver->device_driver_write(fd, buf, n);
	}
	mutex_unlock(&block_device_driver_table[major(fd->rdev)].lock);
	return ret;
}

off_t block_device_driver_lseek(fd_t *fd, off_t offset, int whence)
{
	off_t ret = 0;
	device_driver_t *device_driver;
	if (major(fd->rdev) < 0 || major(fd->rdev) >= DEVICE_DRIVER_TABLE_SIZE) {
		return -EINVAL;
	}
	mutex_lock(&block_device_driver_table[major(fd->rdev)].lock);
	device_driver = block_device_driver_table[major(fd->rdev)].device_driver;
	if (device_driver->device_driver_lseek) {
		ret = device_driver->device_driver_lseek(fd, offset, whence);
	}
	mutex_unlock(&block_device_driver_table[major(fd->rdev)].lock);
	return ret;
}

