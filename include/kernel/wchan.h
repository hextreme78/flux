#ifndef KERNEL_WCHAN_H
#define KERNEL_WCHAN_H

#include <kernel/types.h>
#include <kernel/spinlock.h>
#include <kernel/proc.h>

void wchan_sleep(void *wchan, spinlock_t *sl);
void wchan_signal(void *wchan);
void wchan_broadcast(void *wchan);

#endif

