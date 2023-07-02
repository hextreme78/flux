#ifndef KERNEL_MUTEX_H
#define KERNEL_MUTEX_H

#include <kernel/types.h>

typedef struct mutex mutex_t;

#include <kernel/spinlock.h>

struct mutex {
	spinlock_t sl;
	u64 lock;
};

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

#endif

