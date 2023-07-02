#include <kernel/mutex.h>
#include <kernel/sched.h>

void mutex_init(mutex_t *mutex)
{
	spinlock_init(&mutex->sl);
	mutex->lock = 0;
}

void mutex_lock(mutex_t *mutex)
{
	int irqflags;
	spinlock_acquire_irqsave(&mutex->sl, irqflags);
	while (mutex->lock) {
		spinlock_release_irqrestore(&mutex->sl, irqflags);
		sched();
		spinlock_acquire_irqsave(&mutex->sl, irqflags);
	}
	mutex->lock = 1;
	spinlock_release_irqrestore(&mutex->sl, irqflags);
}

void mutex_unlock(mutex_t *mutex)
{
	int irqflags;
	spinlock_acquire_irqsave(&mutex->sl, irqflags);
	mutex->lock = 0;
	spinlock_release_irqrestore(&mutex->sl, irqflags);
}

