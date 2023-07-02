#ifndef KERNEL_SCHED_H
#define KERNEL_SCHED_H

#include <kernel/proc.h>

void scheduler(void);
void sched(void);
void sched_zombie(void);

void context_switch(context_t *old, context_t *new);

#endif

