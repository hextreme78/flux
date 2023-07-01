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

void spinlock_acquire_irq(spinlock_t *sl)
{
	irq_off();
	spinlock_acquire(sl);
}

void spinlock_release_irq(spinlock_t *sl)
{
	spinlock_release(sl);
	irq_on();
}

void spinlock_acquire_irqsave(spinlock_t *sl)
{
	bool irqsave = irq_enabled();
	irq_off();
	spinlock_acquire(sl);
	sl->irqsave = irqsave;
}

void spinlock_release_irqsave(spinlock_t *sl)
{
	bool irqsave = sl->irqsave;
	spinlock_release(sl);
	if (irqsave) {
		irq_on();
	}
}

