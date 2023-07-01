#include <kernel/sched.h>
#include <kernel/irq.h>
#include <kernel/proc.h>
#include <kernel/spinlock.h>

extern proc_t proctable[NPROC];
void context_switch(ctx_t *old, ctx_t *new);

void scheduler(void)
{
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

void context_switch_prepare(ctx_t *old, ctx_t *new)
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
	u64 irqsave = irq_enabled();
	irq_off();

	spinlock_acquire(&curproc()->lock);
	curproc()->state = PROC_STATE_RUNNABLE;

	context_switch(curproc()->context, curcpu()->context);

	curproc()->state = PROC_STATE_RUNNING;	
	spinlock_release(&curproc()->lock);

	if (irqsave) {
		irq_on();
	}
}

void zombie(void)
{
	spinlock_acquire_irq(&curproc()->lock);
	curproc()->state = PROC_STATE_ZOMBIE;
	context_switch(curproc()->context, curcpu()->context);
}

