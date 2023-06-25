#ifndef KERNEL_PLIC_SIFIVE_H
#define KERNEL_PLIC_SIFIVE_H

#include <kernel/platform-virt.h>
#include <kernel/proc.h>
#include <stdint.h>

void plic_init(void);
void plic_hart_init(void);

/* even-numbered contexts refer to harts m-mode */
/* odd-numbered contexts refer to harts s-mode */
/* we are interested in s-mode, so (2 * cpuid() + 1) */

static inline void plic_irq_on(uint32_t irq)
{
	volatile uint32_t *enable_reg = (volatile uint32_t *) (
			(uint8_t *) VIRT_PLIC +
			0x2000 + 0x80 * (2 * cpuid() + 1) +
			4 * (irq / 32)
			);
	*enable_reg |= (1 << (irq % 32));
}

static inline void plic_irq_off(uint32_t irq)
{
	volatile uint32_t *enable_reg = (volatile uint32_t *) (
			(uint8_t *) VIRT_PLIC +
			0x2000 + 0x80 * (2 * cpuid() + 1) +
			4 * (irq / 32)
			);
	*enable_reg &= ~(1 << (irq % 32));
}

static inline uint32_t plic_irq_claim(void)
{
	return *((volatile uint32_t *) ((uint8_t *) VIRT_PLIC + 0x200000 +
				0x1000 * (2 * cpuid() + 1) + 4));
}

static inline void plic_irq_complete(uint32_t irq)
{
	*((volatile uint32_t *) ((uint8_t *) VIRT_PLIC + 0x200000 +
				0x1000 * (2 * cpuid() + 1) + 4)) = irq;
}

static inline void plic_priority_treshold(uint32_t treshold)
{
	*((volatile uint32_t *) ((uint8_t *) VIRT_PLIC + 0x200000 +
				0x1000 * (2 * cpuid() + 1))) = treshold;
}

static inline void plic_source_priority(uint32_t irq, uint32_t prio)
{
	*((volatile uint32_t *) ((uint8_t *) VIRT_PLIC + 4 * irq)) = prio;
}

#endif

