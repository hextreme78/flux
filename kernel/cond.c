#include <kernel/cond.h>
#include <kernel/sched.h>

void cond_init(cond_t *cond)
{
	spinlock_init(&cond->lock);
	cond->wait = 0;
}

void cond_wait(cond_t *cond, mutex_t *mutex)
{
	int irqflags;
	u64 wait;

	spinlock_acquire_irqsave(&cond->lock, irqflags);
	mutex_unlock(mutex);

	wait = cond->wait;
	cond->wait++;

	while (cond->wait > wait) {
		spinlock_release_irqrestore(&cond->lock, irqflags);

		sched();

		spinlock_acquire_irqsave(&cond->lock, irqflags);
	}

	if (cond->wait) {
		cond->wait--;
	}

	mutex_lock(mutex);
	spinlock_release_irqrestore(&cond->lock, irqflags);
}

void cond_signal(cond_t *cond)
{
	int irqflags;
	spinlock_acquire_irqsave(&cond->lock, irqflags);
	if (cond->wait) {
		cond->wait--;
	}
	spinlock_release_irqrestore(&cond->lock, irqflags);
}

void cond_broadcast(cond_t *cond)
{
	int irqflags;
	spinlock_acquire_irqsave(&cond->lock, irqflags);
	cond->wait = 0;
	spinlock_release_irqrestore(&cond->lock, irqflags);
}

