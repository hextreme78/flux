#ifndef KERNEL_DEV_H
#define KERNEL_DEV_H

#include <kernel/types.h>
#include <kernel/errno.h>

typedef struct device_driver device_driver_t;

#include <kernel/proc.h>
#include <kernel/mutex.h>

#define makedev(major, minor) ((((major) & 0xff) << 8) | ((minor) & 0xff))
#define major(dev) (((dev) & 0xff00) >> 8)
#define minor(dev) ((dev) & 0xff)

struct device_driver_table_entry {
	mutex_t lock;
	device_driver_t *device_driver;
};

struct device_driver {
	char *device_driver_name;
	int (*device_driver_init)(void);
	void (*device_driver_cleanup)(void);
	int (*device_driver_open)(fd_t *fd, int flags, mode_t mode);
	int (*device_driver_close)(fd_t *fd);
	ssize_t (*device_driver_read)(fd_t *fd, void *buf, size_t n);
	ssize_t (*device_driver_write)(fd_t *fd, const void *buf, size_t n);
	off_t (*device_driver_lseek)(fd_t *fd, off_t offset, int whence);
	int (*device_driver_fsync)(fd_t *fd);
	int (*device_driver_fdatasync)(fd_t *fd);
};

void dev_init(void);

int character_device_driver_register(int major, device_driver_t *device_driver);
int character_device_driver_unregister(int major);
int character_device_driver_open(fd_t *fd, int flags, mode_t mode);
int character_device_driver_close(fd_t *fd);
ssize_t character_device_driver_read(fd_t *fd, void *buf, size_t n);
ssize_t character_device_driver_write(fd_t *fd, const void *buf, size_t n);
off_t character_device_driver_lseek(fd_t *fd, off_t offset, int whence);
int character_device_driver_fsync(fd_t *fd);
int character_device_driver_fdatasync(fd_t *fd);

int block_device_driver_register(int major, device_driver_t *device_driver);
int block_device_driver_unregister(int major);
int block_device_driver_open(fd_t *fd, int flags, mode_t mode);
int block_device_driver_close(fd_t *fd);
ssize_t block_device_driver_read(fd_t *fd, void *buf, size_t n);
ssize_t block_device_driver_write(fd_t *fd, const void *buf, size_t n);
off_t block_device_driver_lseek(fd_t *fd, off_t offset, int whence);
int block_device_driver_fsync(fd_t *fd);
int block_device_driver_fdatasync(fd_t *fd);

#endif

