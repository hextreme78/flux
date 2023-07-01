#ifndef KERNEL_WCHAN_H
#define KERNEL_WCHAN_H

#include <kernel/spinlock.h>
#include <kernel/types.h>

typedef struct {
	spinlock_t lock;
	u64 wait;
} wchan_t;

void wchan_init(wchan_t *wchan);
void wchan_wait(wchan_t *wchan, spinlock_t *sl);
void wchan_signal(wchan_t *wchan);
void wchan_broadcast(wchan_t *wchan);

#endif

