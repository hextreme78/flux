#ifndef KERNEL_PLIC_SIFIVE_H
#define KERNEL_PLIC_SIFIVE_H

#include <kernel/platform-virt.h>
#include <kernel/proc.h>
#include <kernel/types.h>

void plic_init(void);
void plic_hart_init(void);

/* even-numbered contexts refer to harts m-mode */
/* odd-numbered contexts refer to harts s-mode */
/* we are interested in s-mode, so (2 * cpuid() + 1) */

static inline void plic_irq_on(u32 irq)
{
	volatile u32 *enable_reg = (volatile u32 *) (
			(u8 *) VIRT_PLIC +
			0x2000 + 0x80 * (2 * cpuid() + 1) +
			4 * (irq / 32)
			);
	*enable_reg |= (1 << (irq % 32));
}

static inline void plic_irq_off(u32 irq)
{
	volatile u32 *enable_reg = (volatile u32 *) (
			(u8 *) VIRT_PLIC +
			0x2000 + 0x80 * (2 * cpuid() + 1) +
			4 * (irq / 32)
			);
	*enable_reg &= ~(1 << (irq % 32));
}

static inline u32 plic_irq_claim(void)
{
	return *((volatile u32 *) ((u8 *) VIRT_PLIC + 0x200000 +
				0x1000 * (2 * cpuid() + 1) + 4));
}

static inline void plic_irq_complete(u32 irq)
{
	*((volatile u32 *) ((u8 *) VIRT_PLIC + 0x200000 +
				0x1000 * (2 * cpuid() + 1) + 4)) = irq;
}

static inline void plic_priority_treshold(u32 treshold)
{
	*((volatile u32 *) ((u8 *) VIRT_PLIC + 0x200000 +
				0x1000 * (2 * cpuid() + 1))) = treshold;
}

static inline void plic_source_priority(u32 irq, u32 prio)
{
	*((volatile u32 *) ((u8 *) VIRT_PLIC + 4 * irq)) = prio;
}

#endif

