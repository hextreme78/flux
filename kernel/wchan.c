#include <kernel/wchan.h>
#include <kernel/sched.h>
#include <kernel/irq.h>

void wchan_sleep(void *wchan, spinlock_t *sl)
{
	int irqflags;
	spinlock_acquire_irqsave(&curproc()->lock, irqflags);
	spinlock_release(sl);

	curproc()->state = PROC_STATE_STOPPED;
	curproc()->wchan = wchan;

	context_switch(curproc()->context, curcpu()->context);

	curproc()->state = PROC_STATE_RUNNING;
	curproc()->wchan = NULL;

	spinlock_acquire(sl);
	spinlock_release_irqrestore(&curproc()->lock, irqflags);
}

void wchan_signal(void *wchan)
{
	extern proc_t proctable[NPROC];
	int irqflags;
	for (size_t i = 0; i < NPROC; i++) {
		spinlock_acquire_irqsave(&proctable[i].lock, irqflags);
		if (proctable[i].state == PROC_STATE_STOPPED &&
				proctable[i].wchan == wchan) {
			proctable[i].state = PROC_STATE_RUNNABLE;
			spinlock_release_irqrestore(&proctable[i].lock, irqflags);
			return;
		}
		spinlock_release_irqrestore(&proctable[i].lock, irqflags);
	}
}

void wchan_broadcast(void *wchan)
{
	extern proc_t proctable[NPROC];
	int irqflags;
	for (size_t i = 0; i < NPROC; i++) {
		spinlock_acquire_irqsave(&proctable[i].lock, irqflags);
		if (proctable[i].state == PROC_STATE_STOPPED &&
				proctable[i].wchan == wchan) {
			proctable[i].state = PROC_STATE_RUNNABLE;
		}
		spinlock_release_irqrestore(&proctable[i].lock, irqflags);
	}
}

