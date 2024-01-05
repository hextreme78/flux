#ifndef KERNEL_CDEV_TTY_H
#define KERNEL_CDEV_TTY_H

#include <kernel/dev.h>

extern struct device_driver cdev_tty;

#define TTY_CONSOLE makedev(TTY_CONSOLE_MAJOR, TTY_CONSOLE_MINOR)
#define TTY_STDIN   makedev(CDEV_TTY_MAJOR, CDEV_TTY_STDIN)
#define TTY_STDOUT  makedev(CDEV_TTY_MAJOR, CDEV_TTY_STDOUT)
#define TTY_STDERR  makedev(CDEV_TTY_MAJOR, CDEV_TTY_STDERR)

#define CDEV_TTY_MAJOR 2

#define CDEV_TTY_CONSOLE 0
#define CDEV_TTY_TTY     1
#define CDEV_TTY_STDIN   2
#define CDEV_TTY_STDOUT  3
#define CDEV_TTY_STDERR  4
#define CDEV_TTY_TTY0    5
#define CDEV_TTY_TTYS0   6

#endif

