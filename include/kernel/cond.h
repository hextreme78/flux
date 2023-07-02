#ifndef KERNEL_COND_H
#define KERNEL_COND_H

#include <kernel/types.h>

typedef struct cond cond_t;

#include <kernel/mutex.h>

struct cond {
	spinlock_t lock;
	u64 wait;
};

void cond_init(cond_t *cond);
void cond_wait(cond_t *cond, mutex_t *mutex);
void cond_signal(cond_t *cond);
void cond_broadcast(cond_t *cond);

#endif

