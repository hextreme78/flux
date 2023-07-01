#include <kernel/mutex.h>
#include <kernel/sched.h>

void mutex_init(mutex_t *mutex)
{
	spinlock_init(&mutex->sl);
	mutex->lock = 0;
}

void mutex_lock(mutex_t *mutex)
{
	spinlock_acquire_irqsave(&mutex->sl);
	while (mutex->lock) {
		spinlock_release_irqsave(&mutex->sl);
		sched();
		spinlock_acquire_irqsave(&mutex->sl);
	}
	mutex->lock = 1;
	spinlock_release_irqsave(&mutex->sl);
}

void mutex_unlock(mutex_t *mutex)
{
	spinlock_acquire_irqsave(&mutex->sl);
	mutex->lock = 0;
	spinlock_release_irqsave(&mutex->sl);
}

