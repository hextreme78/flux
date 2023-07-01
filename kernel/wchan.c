#include <kernel/wchan.h>
#include <kernel/sched.h>

void wchan_init(wchan_t *wchan)
{
	spinlock_init(&wchan->lock);
	wchan->wait = 0;
}

void wchan_wait(wchan_t *wchan, spinlock_t *sl)
{
	u64 wait;
	spinlock_release(sl);

	spinlock_acquire(&wchan->lock);
	wait = wchan->wait;
	wchan->wait++;

	while (wchan->wait > wait) {
		spinlock_release(&wchan->lock);

		sched();

		spinlock_acquire(&wchan->lock);
	}

	if (wchan->wait) {
		wchan->wait--;
	}

	spinlock_release(&wchan->lock);

	spinlock_acquire(sl);
}

void wchan_signal(wchan_t *wchan)
{
	spinlock_acquire(&wchan->lock);
	if (wchan->wait) {
		wchan->wait--;
	}
	spinlock_release(&wchan->lock);
}

void wchan_broadcast(wchan_t *wchan)
{
	spinlock_acquire(&wchan->lock);
	wchan->wait = 0;
	spinlock_release(&wchan->lock);
}

