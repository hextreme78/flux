#ifndef KERNEL_CDEV_MEM_H
#define KERNEL_CDEV_MEM_H

#include <kernel/dev.h>

extern struct device_driver cdev_mem;

#define CDEV_MEM_MAJOR 1

#define CDEV_MEM_MEM     0
#define CDEV_MEM_PORT    1
#define CDEV_MEM_KMEM    2
#define CDEV_MEM_NULL    3
#define CDEV_MEM_ZERO    4
#define CDEV_MEM_FULL    5
#define CDEV_MEM_RANDOM  6
#define CDEV_MEM_URANDOM 7
#define CDEV_MEM_KMSG    8

#endif

