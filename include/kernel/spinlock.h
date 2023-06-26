#ifndef KERNEL_SPINLOCK_H
#define KERNEL_SPINLOCK_H

#include <kernel/types.h>

typedef volatile struct {
	u64 lock;
} spinlock_t;

void spinlock_init(spinlock_t *sl);
void spinlock_acquire(spinlock_t *sl);
void spinlock_release(spinlock_t *sl);

#endif

