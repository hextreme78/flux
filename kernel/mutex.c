#include <kernel/mutex.h>
#include <kernel/proc.h>

void mutex_init(mutex_t *mutex)
{
	spinlock_init(&mutex->sl);
	mutex->lock = 0;
}

void mutex_lock(mutex_t *mutex)
{
	spinlock_acquire(&mutex->sl);
	while (mutex->lock) {
		spinlock_release(&mutex->sl);

		sched_yield();

		spinlock_acquire(&mutex->sl);
	}
	mutex->lock = 1;
	spinlock_release(&mutex->sl);
}

void mutex_unlock(mutex_t *mutex)
{
	spinlock_acquire(&mutex->sl);
	mutex->lock = 0;
	spinlock_release(&mutex->sl);
}

