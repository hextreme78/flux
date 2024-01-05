#include <kernel/cdev-tty.h>
#include <kernel/klib.h>
#include <abi-bits/fcntl.h>
#include <kernel/mutex.h>
#include <kernel/uart-ns16550a.h>

static int cdev_tty_init(void);
static int cdev_tty_open(fd_t *fd, int flags, mode_t mode);
static ssize_t cdev_tty_read(fd_t *fd, void *buf, size_t n);
static ssize_t cdev_tty_write(fd_t *fd, const void *buf, size_t n);
static ssize_t cdev_tty_lseek(fd_t *fd, off_t offset, int whence);
static int cdev_tty_fsync(fd_t *fd);

struct device_driver cdev_tty = {
	.device_driver_name = "tty",
	.device_driver_init = cdev_tty_init,
	.device_driver_open = cdev_tty_open,
	.device_driver_read = cdev_tty_read,
	.device_driver_write = cdev_tty_write,
	.device_driver_lseek = cdev_tty_lseek,
	.device_driver_fsync = cdev_tty_fsync,
	.device_driver_fdatasync = cdev_tty_fsync
};

static dev_t active_tty_dev = TTY_CONSOLE;
static mutex_t ttys0_read_mutex, ttys0_write_mutex;

static int cdev_tty_init(void)
{
	mutex_init(&ttys0_read_mutex);
	mutex_init(&ttys0_write_mutex);
	return 0;
}

static int tty_open(dev_t rdev, int flags)
{
	if (!(flags & O_NOCTTY)) {
		bool is_session_leader;
		int irqflags;
		pid_t sid;
		spinlock_acquire_irqsave(&curproc()->lock, irqflags);
		sid = curproc()->sid;
		is_session_leader = (curproc()->pid == curproc()->sid);
		spinlock_release_irqrestore(&curproc()->lock, irqflags);

		if (is_session_leader) {
			curproc()->ctty = rdev;
			/* not implemented
			 * should update ctty for each process in session
			 */
		}
	}
	return 0;
}

static int cdev_tty_open(fd_t *fd, int flags, mode_t mode)
{
	ssize_t ret = 0;
	switch (minor(fd->rdev)) {
	case CDEV_TTY_TTY0:
		ret = tty_open(active_tty_dev, flags);
		break;
	case CDEV_TTY_CONSOLE:
		ret = tty_open(TTY_CONSOLE, flags);
		break;
	case CDEV_TTY_TTY:
	case CDEV_TTY_STDIN:
	case CDEV_TTY_STDOUT:
	case CDEV_TTY_STDERR:
		break;
	case CDEV_TTY_TTYS0:
		ret = tty_open(fd->rdev, flags);
		break;
	default:
		ret = -ENODEV;
	}
	return ret;
}

static ssize_t tty_read(dev_t rdev, fd_t *fd, void *buf, size_t n)
{
	if (rdev == makedev(CDEV_TTY_MAJOR, CDEV_TTY_TTYS0)) {
		mutex_lock(&ttys0_read_mutex);
		for (size_t i = 0; i < n; i++) {
			char ch = uart_getch_async();
			if (copy_to_user(buf + i, &ch, sizeof(ch))) {
				mutex_unlock(&ttys0_read_mutex);
				return -EFAULT;
			}
		}
		mutex_unlock(&ttys0_read_mutex);
		return n;
	}
	return -ENODEV;
}

static ssize_t cdev_tty_read(fd_t *fd, void *buf, size_t n)
{
	ssize_t ret = 0;
	switch (minor(fd->rdev)) {
	case CDEV_TTY_TTY0:
		ret = tty_read(active_tty_dev, fd, buf, n);
		break;
	case CDEV_TTY_CONSOLE:
		ret = tty_read(TTY_CONSOLE, fd, buf, n);
		break;
	case CDEV_TTY_TTY:
	case CDEV_TTY_STDIN:
		ret = tty_read(curproc()->ctty, fd, buf, n);
		break;
	case CDEV_TTY_STDOUT:
	case CDEV_TTY_STDERR:
		ret = -EINVAL;
		break;
	case CDEV_TTY_TTYS0:
		ret = tty_read(fd->rdev, fd, buf, n);
		break;
	default:
		ret = -ENODEV;
	}
	return ret;
}

static ssize_t tty_write(dev_t rdev, bool buffered, fd_t *fd, const void *buf, size_t n)
{
	if (rdev == makedev(CDEV_TTY_MAJOR, CDEV_TTY_TTYS0)) {
		mutex_lock(&ttys0_write_mutex);
		for (size_t i = 0; i < n; i++) {
			char ch;
			if (copy_from_user(&ch, buf + i, sizeof(ch))) {
				mutex_unlock(&ttys0_write_mutex);
				return -EFAULT;
			}
			if (buffered) {
				uart_putch_async(ch);
			} else {
				uart_putch_sync(ch);
			}
		}
		mutex_unlock(&ttys0_write_mutex);
		return n;
	}
	return -ENODEV;
}

static ssize_t cdev_tty_write(fd_t *fd, const void *buf, size_t n)
{
	ssize_t ret = 0;
	switch (minor(fd->rdev)) {
	case CDEV_TTY_TTY0:
		ret = tty_write(active_tty_dev, true, fd, buf, n);
		break;
	case CDEV_TTY_CONSOLE:
		ret = tty_write(TTY_CONSOLE, true, fd, buf, n);
		break;
	case CDEV_TTY_STDIN:
		ret = -EINVAL;
		break;
	case CDEV_TTY_TTY:
	case CDEV_TTY_STDOUT:
		ret = tty_write(curproc()->ctty, true, fd, buf, n);
		break;
	case CDEV_TTY_STDERR:
		ret = tty_write(curproc()->ctty, false, fd, buf, n);
		break;
	case CDEV_TTY_TTYS0:
		ret = tty_write(fd->rdev, true, fd, buf, n);
		break;
	default:
		ret = -ENODEV;
	}
	return ret;
}

static ssize_t cdev_tty_lseek(fd_t *fd, off_t offset, int whence)
{
	return -ESPIPE;
}

static int tty_fsync(dev_t rdev)
{
	if (rdev == makedev(CDEV_TTY_MAJOR, CDEV_TTY_TTYS0)) {
		mutex_lock(&ttys0_write_mutex);
		uart_tx_flush_sync();
		mutex_unlock(&ttys0_write_mutex);
		return 0;
	}
	return -ENODEV;
}

static int cdev_tty_fsync(fd_t *fd)
{
	ssize_t ret = 0;
	switch (minor(fd->rdev)) {
	case CDEV_TTY_TTY0:
		ret = tty_fsync(active_tty_dev);
		break;
	case CDEV_TTY_CONSOLE:
		ret = tty_fsync(TTY_CONSOLE);
		break;
	case CDEV_TTY_STDIN:
		ret = -EINVAL;
		break;
	case CDEV_TTY_TTY:
	case CDEV_TTY_STDOUT:
		ret = tty_fsync(curproc()->ctty);
		break;
	case CDEV_TTY_STDERR:
		break;
	case CDEV_TTY_TTYS0:
		ret = tty_fsync(fd->rdev);
		break;
	default:
		ret = -ENODEV;
	}
	return ret;
}

