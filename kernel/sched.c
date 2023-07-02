#include <kernel/sched.h>
#include <kernel/irq.h>
#include <kernel/proc.h>
#include <kernel/spinlock.h>

void scheduler(void)
{
	extern proc_t proctable[NPROC];
	irq_on();
	while (1) {
		for (size_t i = 0; i < NPROC; i++) {
			spinlock_acquire_irq(&proctable[i].lock);
			if (proctable[i].state == PROC_STATE_RUNNABLE) {
				curcpu()->proc = &proctable[i];

				context_switch(curcpu()->context, curproc()->context);

				curcpu()->proc = NULL;
			}
			spinlock_release_irq(&proctable[i].lock);
		}
	}
}

void context_switch_prepare(context_t *old, context_t *new)
{
	/* we will enter s-mode and interrupts will be disabled */
	w_sstatus((r_sstatus() & ~SSTATUS_SPIE) | SSTATUS_SPP);

	/* we will return after context_switch call */
	w_sepc(new->ra);

	/* set new kernel pagetable */
	sfence_vma();
	w_satp(PA_TO_PN(new->kpagetable) | SATP_MODE_SV39);
	sfence_vma();
}

void sched(void)
{
	int irqflags;
	spinlock_acquire_irqsave(&curproc()->lock, irqflags);
	curproc()->state = PROC_STATE_RUNNABLE;

	context_switch(curproc()->context, curcpu()->context);

	curproc()->state = PROC_STATE_RUNNING;	
	spinlock_release_irqrestore(&curproc()->lock, irqflags);
}

void sched_zombie(void)
{
	spinlock_acquire_irq(&curproc()->lock);
	curproc()->state = PROC_STATE_ZOMBIE;
	context_switch(curproc()->context, curcpu()->context);
}

