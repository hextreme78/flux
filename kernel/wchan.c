#include <kernel/wchan.h>
#include <kernel/proc.h>
#include <kernel/irq.h>

void switch_to_scheduler_and_save(ctx_t *kcontext, ctx_t *pcontext);
void kerneltrap(void);
extern i64 irq_cnt[NCPU];

void wchan_sleep(void)
{
	/* save irq state */
	irq_save();

	/* Interrupts will be disabled on sret
	 * because we must hold proccess lock
	 */
	irq_cnt[cpuid()] = 1;
	spinlock_acquire(&curproc()->lock);
	w_sstatus((r_sstatus() | SSTATUS_SPP_S) & ~SSTATUS_SPIE);

	/* route interrupts to kernel_irq_handler */
	w_stvec((u64) kerneltrap | STVEC_MODE_DIRECT);

	/* we must return after switch_to_user call in scheduler */
	w_sepc((u64) curcpu()->context->ra);

	/* set proc state */
	curproc()->state = PROC_STATE_RUNNABLE;

	/* return to scheduler */
	switch_to_scheduler_and_save(curcpu()->context, curproc()->context);

	/* restore irq state */
	irq_restore();
}

void wchan_init(wchan_t *wchan)
{
	spinlock_init(&wchan->lock);
	wchan->wait = 0;
}

void wchan_wait(wchan_t *wchan, spinlock_t *sl)
{
	u64 wait;
	spinlock_release(sl);

	spinlock_acquire(&wchan->lock);
	wait = wchan->wait;
	wchan->wait++;

	while (wchan->wait > wait) {
		spinlock_release(&wchan->lock);

		wchan_sleep();

		spinlock_acquire(&wchan->lock);
	}

	if (wchan->wait) {
		wchan->wait--;
	}

	spinlock_release(&wchan->lock);

	spinlock_acquire(sl);
}

void wchan_signal(wchan_t *wchan)
{
	spinlock_acquire(&wchan->lock);
	if (wchan->wait) {
		wchan->wait--;
	}
	spinlock_release(&wchan->lock);
}

void wchan_broadcast(wchan_t *wchan)
{
	spinlock_acquire(&wchan->lock);
	wchan->wait = 0;
	spinlock_release(&wchan->lock);
}

