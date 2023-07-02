#include <kernel/spinlock.h>
#include <kernel/riscv64.h>
#include <kernel/irq.h>

void spinlock_init(spinlock_t *sl)
{
	sl->lock = 0;
}

void spinlock_acquire(spinlock_t *sl)
{
	while (atomic_test_and_set(&sl->lock, 1));
	atomic_acquire_membar();
}

void spinlock_release(spinlock_t *sl)
{
	atomic_release_membar();
	atomic_set(&sl->lock, 0);
}

