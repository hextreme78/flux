#ifndef KERNEL_IRQ_H
#define KERNEL_IRQ_H

#include <kernel/proc.h>

void irq_init(void);
void irq_hart_init(void);
void irq_on(void);
void irq_off(void);

void irq_save(void);
void irq_restore(void);

void user_irq_handler(void);

#endif

