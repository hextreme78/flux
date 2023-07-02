#ifndef KERNEL_IRQ_H
#define KERNEL_IRQ_H

void irq_hart_init(void);

#include <kernel/proc.h>

static inline void irq_on(void)
{
	w_sstatus(r_sstatus() | SSTATUS_SIE);
}

static inline void irq_off(void)
{
	w_sstatus(r_sstatus() & ~SSTATUS_SIE);
}

static inline bool irq_enabled(void)
{
	return r_sstatus() & SSTATUS_SIE;
}

#endif

