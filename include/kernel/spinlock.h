#ifndef KERNEL_SPINLOCK_H
#define KERNEL_SPINLOCK_H

#include <kernel/types.h>

typedef volatile struct spinlock spinlock_t;

struct spinlock {
	u64 lock;
};

#include <kernel/irq.h>

void spinlock_init(spinlock_t *sl);
void spinlock_acquire(spinlock_t *sl);
void spinlock_release(spinlock_t *sl);
void spinlock_acquire_irq(spinlock_t *sl);
void spinlock_release_irq(spinlock_t *sl);
void spinlock_acquire_irqsave(spinlock_t *sl, int *flags);
void spinlock_release_irqrestore(spinlock_t *sl, int flags);

#define spinlock_acquire_irq(sl) \
	({ \
		irq_off(); \
		spinlock_acquire(sl); \
	})

#define spinlock_release_irq(sl) \
	({ \
		spinlock_release(sl); \
		irq_on(); \
	})


#define spinlock_acquire_irqsave(sl, flags) \
	({ \
		(flags) = irq_enabled(); \
		irq_off(); \
		spinlock_acquire(sl); \
	})

#define spinlock_release_irqrestore(sl, flags) \
	({ \
		spinlock_release(sl); \
		if (flags) { \
			irq_on(); \
		} \
	})

#endif

