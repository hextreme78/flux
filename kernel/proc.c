#include <kernel/proc.h>
#include <kernel/alloc.h>
#include <kernel/irq.h>
#include <kernel/vm.h>
#include <kernel/kprintf.h>
#include <kernel/elf.h>

extern u64 trampoline;

void usertrap(void);
void switch_to_user(ctx_t *kcontext);
void switch_userret(ctx_t *pcontext, u64 satp);
void switch_irqret(ctx_t *pcontext);

cpu_t cpus[NCPU];

static spinlock_t nextpid_lock;
static volatile pid_t nextpid = 1;

static spinlock_t proctable_lock;
static proc_t proctable[NPROC];

void proc_init(void)
{
	spinlock_init(&nextpid_lock);
	spinlock_init(&proctable_lock);
	for (size_t i = 0; i < NPROC; i++) {
		spinlock_init(&proctable[i].lock);
		proctable[i].state = PROC_STATE_KILLED;
		proctable[i].pid = 0;
	}
}

void proc_hart_init(void)
{
	cpus[cpuid()].proc = NULL;
	cpus[cpuid()].context = kmalloc(sizeof(ctx_t));
	if (!cpus[cpuid()].context) {
		panic("proc_hart_init failed. no memory.");
	}
}

void scheduler(void)
{
	/* round-robin scheduler */
	irq_on();
	while (1) {
		for (size_t i = 0; i < NPROC; i++) {
			spinlock_acquire(&proctable[i].lock);
			if (proctable[i].state == PROC_STATE_RUNNABLE) {
				curcpu()->proc = &proctable[i];

				switch_to_user(curcpu()->context);
	
				curcpu()->proc = NULL;
			}
			spinlock_release(&proctable[i].lock);
		}
	}
}

void switch_userret_prepare(void)
{
	u64 satp = SATP_MODE_SV39 | PA_TO_PN(curproc()->pagetable);
	void (*userret)(ctx_t *context, u64 satp) = (void *)
		(VA_TRAMPOLINE + (u64) switch_userret -
		(u64) &trampoline);

	/* set proc state */
	curproc()->state = PROC_STATE_RUNNING;

	/* release process lock acquired in scheduler */
	spinlock_release(&curproc()->lock);

	/* disable irq while switching */
	w_sstatus(r_sstatus() & ~SSTATUS_SIE);

	/* user epc address for sret */
	w_sepc(curproc()->context->epc);

	if (curproc()->in_irq) {
		/* Set spp to s-mode for sret and reset
		 * spie to disable interrupts after sret
		 */
		w_sstatus((r_sstatus() & ~SSTATUS_SPIE) | SSTATUS_SPP_S);

		/* return to irq handler */
		switch_irqret(curproc()->context);
	}

	/* set usertrap
	 * usertrap is mapped to the highest page in addrspace
	 */
	w_stvec(VA_TRAMPOLINE + (u64) usertrap - (u64) &trampoline);

	/* Set spp to u-mode for sret and set
	 * spie to enable interrupts after sret
	 */
	w_sstatus((r_sstatus() & ~SSTATUS_SPP_S) | SSTATUS_SPIE);

	/* save cpuid in trapframe */
	curproc()->trapframe->kernel_tp = cpuid();

	/* return to u-mode */
	userret(curproc()->context, satp);
}

static pid_t pid_alloc(void)
{
	pid_t pid;
	spinlock_acquire(&nextpid_lock);
	if (nextpid > PIDMAX) {
		/* we reached maximum pid, so no more pids */
		spinlock_release(&nextpid_lock);
		return 0;
	}
	pid = nextpid;
	nextpid++;
	spinlock_release(&nextpid_lock);
	return pid;
}

static proc_t *proc_slot_alloc(void)
{
	size_t i;
	proc_t *proc = NULL;
	
	for (i = 0; i < NPROC; i++) {
		spinlock_acquire(&proctable[i].lock);
		if (proctable[i].state == PROC_STATE_KILLED) {
			proc = &proctable[i];
			proc->state = PROC_STATE_PREPARING;
			spinlock_release(&proctable[i].lock);
			break;
		}
		spinlock_release(&proctable[i].lock);
	}

	if (i == NPROC) {
		return NULL;
	}

	proc->context = NULL;
	proc->trapframe = NULL;
	proc->kstack = NULL;
	proc->ustack = NULL;
	proc->pagetable = NULL;
	list_init(&proc->segment_list.segments);

	proc->parent = NULL;
	list_init(&proc->children);

	proc->in_irq = false;

	return proc;
}

void proc_destroy(proc_t *proc)
{
	spinlock_acquire(&proc->lock);
	proc->state = PROC_STATE_PREPARING;
	spinlock_release(&proc->lock);

	proc->pid = 0;

	if (proc->context) {
		kfree(proc->context);	
	}

	if (proc->kstack) {
		kpage_free(proc->kstack);
	}

	if (proc->ustack) {
		kpage_free(proc->ustack);
	}

	if (proc->trapframe) {
		kpage_free(proc->trapframe);
	}

	if (proc->pagetable) {
		vm_pageunmap(proc->pagetable, PA_TO_PN(VA_TRAMPOLINE));
		vm_pageunmap(proc->pagetable, PA_TO_PN(VA_TRAPFRAME));
		vm_pageunmap_range(proc->pagetable, PA_TO_PN(VA_USTACK), USTACKNPAGES);
		vm_pageunmap(proc->pagetable, PA_TO_PN(VA_USTACK_GUARD));

		/* unmap and free segments */
		while (!list_empty(&proc->segment_list.segments)) {
			segment_t *segment = list_next_entry(
					&proc->segment_list,
					segments);
			vm_pageunmap_range(proc->pagetable,
					PA_TO_PN(segment->vstart),
					segment->vlen / PAGESZ);
			kpage_free((void *) segment->pstart);

			list_del(&segment->segments);
			kfree(segment);
		}

		kpage_free(proc->pagetable);
	}

	spinlock_acquire(&proc->lock);
	proc->state = PROC_STATE_KILLED;
	spinlock_release(&proc->lock);
}

int proc_create(void *elf, size_t elfsz)
{
	int err = 0;
	proc_t *proc;
	pid_t pid;

	proc = proc_slot_alloc();
	if (!proc) {
		return -EBUSY;
	}

	pid = pid_alloc();
	if (!pid) {
		proc_destroy(proc);
		return -EBUSY;
	}
	proc->pid = pid;

	proc->context = kmalloc(sizeof(ctx_t));
	if (!proc->context) {
		proc_destroy(proc);
		return -ENOMEM;
	}

	proc->pagetable = kpage_alloc(1);
	if (!proc->pagetable) {
		proc_destroy(proc);
		return -ENOMEM;
	}
	vm_pagetable_init(proc->pagetable);

	proc->kstack = kpage_alloc(KSTACKNPAGES);
	if (!proc->kstack) {
		proc_destroy(proc);
		return -ENOMEM;
	}

	proc->trapframe = kpage_alloc(1);
	if (!proc->trapframe) {
		proc_destroy(proc);	
		return -ENOMEM;
	}
	proc->trapframe->kernel_sp = (u64) proc->kstack + KSTACKNPAGES * PAGESZ;
	proc->trapframe->kernel_tp = cpuid();
	proc->trapframe->kernel_satp = r_satp();
	proc->trapframe->user_irq_handler = (u64) user_irq_handler;

	proc->ustack = kpage_alloc(USTACKNPAGES);
	if (!proc->ustack) {
		proc_destroy(proc);	
		return -ENOMEM;
	}

	err = vm_pagemap(proc->pagetable, PTE_X | PTE_G,
			PA_TO_PN(VA_TRAMPOLINE),
			PA_TO_PN(&trampoline));
	if (err) {
		proc_destroy(proc);
		return err;
	}

	err = vm_pagemap(proc->pagetable, PTE_R | PTE_W,
			PA_TO_PN(VA_TRAPFRAME),
			PA_TO_PN(proc->trapframe));
	if (err) {
		proc_destroy(proc);
		return err;
	}

	err = vm_pagemap_range(proc->pagetable, PTE_R | PTE_W | PTE_U,
			PA_TO_PN(VA_USTACK),
			PA_TO_PN(proc->ustack),
			USTACKNPAGES);
	if (err) {
		proc_destroy(proc);
		return err;
	}

	err = vm_pagemap(proc->pagetable, 0, PA_TO_PN(VA_USTACK_GUARD), 0);
	if (err) {
		proc_destroy(proc);
		return err;
	}

	err = elf_load(proc, elf, elfsz);
	if (err) {
		proc_destroy(proc);
		return err;
	}

	proc->context->sp = (u64) VA_USTACK + USTACKNPAGES * PAGESZ;

	spinlock_acquire(&proc->lock);
	proc->state = PROC_STATE_RUNNABLE;
	spinlock_release(&proc->lock);

	return 0;
}

void sched_yield(void)
{

}

