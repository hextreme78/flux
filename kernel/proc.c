#include <kernel/proc.h>
#include <kernel/alloc.h>
#include <kernel/irq.h>
#include <kernel/vm.h>
#include <kernel/kprintf.h>
#include <kernel/elf.h>
#include <kernel/trampoline.h>

static spinlock_t nextpid_lock;
static volatile pid_t nextpid = 1;

cpu_t cpus[NCPU];
proc_t proctable[NPROC];

void proc_init(void)
{
	spinlock_init(&nextpid_lock);
	for (size_t i = 0; i < NPROC; i++) {
		spinlock_init(&proctable[i].lock);
		proctable[i].state = PROC_STATE_KILLED;
		proctable[i].pid = 0;
	}
}

void proc_hart_init(void)
{
	extern pte_t kpagetable[PTE_MAX];
	curcpu()->proc = NULL;
	curcpu()->context = kmalloc(sizeof(*curcpu()->context));
	if (!curcpu()->context) {
		panic("no memory");
	}
	curcpu()->context->kpagetable = (u64) kpagetable;
}

static pid_t pid_alloc(void)
{
	int irqflags;
	pid_t pid;
	spinlock_acquire_irqsave(&nextpid_lock, irqflags);
	if (nextpid > PIDMAX) {
		/* we reached maximum pid, so no more pids */
		spinlock_release_irqrestore(&nextpid_lock, irqflags);
		return 0;
	}
	pid = nextpid;
	nextpid++;
	spinlock_release_irqrestore(&nextpid_lock, irqflags);
	return pid;
}

static proc_t *proc_slot_alloc(void)
{
	int irqflags;
	size_t i;
	proc_t *proc = NULL;

	for (i = 0; i < NPROC; i++) {
		spinlock_acquire_irqsave(&proctable[i].lock, irqflags);
		if (proctable[i].state == PROC_STATE_KILLED) {
			proc = &proctable[i];
			proc->state = PROC_STATE_PREPARING;
			spinlock_release_irqrestore(&proctable[i].lock, irqflags);
			break;
		}
		spinlock_release_irqrestore(&proctable[i].lock, irqflags);
	}

	if (i == NPROC) {
		return NULL;
	}

	proc->context = NULL;
	proc->trapframe = NULL;
	proc->kstack = NULL;
	proc->ustack = NULL;
	proc->upagetable = NULL;
	proc->kpagetable = NULL;
	list_init(&proc->segment_list.segments);

	proc->parent = NULL;
	list_init(&proc->children);

	proc->errno = 0;

	proc->wchan = NULL;

	return proc;
}

void proc_destroy(proc_t *proc)
{
	int irqflags;
	spinlock_acquire_irqsave(&proc->lock, irqflags);
	proc->state = PROC_STATE_PREPARING;
	spinlock_release_irqrestore(&proc->lock, irqflags);

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

	if (proc->upagetable) {
		vm_pageunmap(proc->upagetable, PA_TO_PN(VA_TRAMPOLINE));
		vm_pageunmap(proc->upagetable, PA_TO_PN(VA_TRAPFRAME));
		vm_pageunmap_range(proc->upagetable, PA_TO_PN(VA_USTACK), USTACKNPAGES);
		vm_pageunmap(proc->upagetable, PA_TO_PN(VA_USTACK_GUARD));

		/* unmap and free segments */
		while (!list_empty(&proc->segment_list.segments)) {
			segment_t *segment = list_next_entry(
					&proc->segment_list,
					segments);
			vm_pageunmap_range(proc->upagetable,
					PA_TO_PN(segment->vstart),
					segment->vlen / PAGESZ);
			kpage_free((void *) segment->pstart);

			list_del(&segment->segments);
			kfree(segment);
		}

		kpage_free(proc->upagetable);
	}

	if (proc->kpagetable) {
		vm_pageunmap(proc->kpagetable, PA_TO_PN(VA_TRAPFRAME));
		vm_pageunmap_kpagetable(proc->kpagetable);

		kpage_free(proc->kpagetable);
	}

	spinlock_acquire_irqsave(&proc->lock, irqflags);
	proc->state = PROC_STATE_KILLED;
	spinlock_release_irqrestore(&proc->lock, irqflags);
}

int proc_create(void *elf, size_t elfsz)
{
	int irqflags;
	void user_irq_handler();
	void kerneltrap(void);
	void userret(void);

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

	proc->context = kmalloc(sizeof(context_t));
	if (!proc->context) {
		proc_destroy(proc);
		return -ENOMEM;
	}

	proc->upagetable = kpage_alloc(1);
	if (!proc->upagetable) {
		proc_destroy(proc);
		return -ENOMEM;
	}
	vm_pagetable_init(proc->upagetable);

	proc->kpagetable = kpage_alloc(1);
	if (!proc->kpagetable) {
		proc_destroy(proc);
		return -ENOMEM;
	}
	vm_pagetable_init(proc->kpagetable);

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

	proc->ustack = kpage_alloc(USTACKNPAGES);
	if (!proc->ustack) {
		proc_destroy(proc);	
		return -ENOMEM;
	}

	err = vm_pagemap(proc->upagetable, PTE_X | PTE_G,
			PA_TO_PN(VA_TRAMPOLINE),
			PA_TO_PN(&trampoline));
	if (err) {
		proc_destroy(proc);
		return err;
	}

	err = vm_pagemap(proc->upagetable, PTE_R | PTE_W,
			PA_TO_PN(VA_TRAPFRAME),
			PA_TO_PN(proc->trapframe));
	if (err) {
		proc_destroy(proc);
		return err;
	}

	err = vm_pagemap_range(proc->upagetable, PTE_R | PTE_W | PTE_U,
			PA_TO_PN(VA_USTACK),
			PA_TO_PN(proc->ustack),
			USTACKNPAGES);
	if (err) {
		proc_destroy(proc);
		return err;
	}

	err = vm_pagemap(proc->upagetable, 0, PA_TO_PN(VA_USTACK_GUARD), 0);
	if (err) {
		proc_destroy(proc);
		return err;
	}

	err = vm_pagemap_kpagetable(proc->kpagetable);
	if (err) {
		proc_destroy(proc);
		return err;
	}

	err = vm_pagemap(proc->kpagetable, PTE_R | PTE_W,
			PA_TO_PN(VA_TRAPFRAME),
			PA_TO_PN(proc->trapframe));
	if (err) {
		proc_destroy(proc);
		return err;
	}

	err = elf_load(proc, elf, elfsz);
	if (err) {
		proc_destroy(proc);
		return err;
	}

	proc->trapframe->sp = (u64) VA_USTACK + USTACKNPAGES * PAGESZ;
	proc->trapframe->kstack = (u64) proc->kstack + KSTACKNPAGES * PAGESZ;
	proc->trapframe->kerneltrap = (u64) kerneltrap;
	proc->trapframe->kpagetable = (u64) proc->kpagetable;
	proc->trapframe->user_irq_handler = (u64) user_irq_handler;
	proc->trapframe->usertrap = (u64) trampoline_usertrap;
	proc->trapframe->upagetable = (u64) proc->upagetable;

	proc->context->sp = proc->trapframe->kstack;
	proc->context->ra = (u64) userret;
	proc->context->kpagetable = (u64) proc->kpagetable;

	spinlock_acquire_irqsave(&proc->lock, irqflags);
	proc->state = PROC_STATE_RUNNABLE;
	spinlock_release_irqrestore(&proc->lock, irqflags);

	return 0;
}

