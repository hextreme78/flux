#ifndef KERNEL_MUTEX_H
#define KERNEL_MUTEX_H

#include <kernel/spinlock.h>
#include <kernel/types.h>

typedef struct {
	spinlock_t sl;
	u64 lock;
} mutex_t;

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

#endif

