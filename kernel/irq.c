#include <kernel/irq.h>
#include <kernel/kprintf.h>
#include <kernel/riscv64.h>
#include <kernel/uart-ns16550a.h>
#include <kernel/plic-sifive.h>
#include <kernel/proc.h>
#include <kernel/syscall.h>

i64 irq_cnt[NCPU];

void kerneltrap(void);
void switch_to_scheduler(ctx_t *kcontext);

void irq_save(void)
{
	curproc()->irq_save = irq_cnt[cpuid()];
	curproc()->in_irq = true;
}

void irq_restore(void)
{
	irq_cnt[cpuid()] = curproc()->irq_save;
	curproc()->in_irq = false;
}

void irq_on(void)
{
	if (irq_cnt[cpuid()] < 0) {
		irq_cnt[cpuid()]++;
	} else if (!irq_cnt[cpuid()]) {
		irq_cnt[cpuid()]++;
		w_sstatus(r_sstatus() | SSTATUS_SIE);
	}
}

void irq_off(void)
{
	w_sstatus(r_sstatus() & ~SSTATUS_SIE);
	irq_cnt[cpuid()]--;
}

static void external_irq_handler(void)
{
	u32 irq = plic_irq_claim();
	switch (irq) {
	case VIRT_PLIC_UART0:
		uart_irq_handler();
		break;
	case 0:
		/* another hart served interrupt */
		break;
	default:
		panic("unknow external irq");
	}
	plic_irq_complete(irq);
}

static void kernel_timer_irq_handler(void)
{
	/* check for panic and spin if true */
	while (paniced);

	/* We need to disable stie bit or timer interrupt
	 * will be triggered again immediately after return
	 * because stip bit is always enabled
	 */
	w_sie(r_sie() & ~SIE_STIE);
}

void kernel_irq_handler(void)
{
	u64 scause = r_scause();
	u64 intr = scause & SCAUSE_INTERRUPT_MASK;
	u64 excode = scause & SCAUSE_EXCEPTION_CODE_MASK;

	/* interrupts are off so decrement irq_cnt */
	irq_cnt[cpuid()]--;

	if (intr) {
		switch (excode) {
		case SCAUSE_TIMER_IRQ:
			kernel_timer_irq_handler();
			break;
		case SCAUSE_SOFTWARE_IRQ:
			panic("SCAUSE_SOFTWARE_IRQ");
		case SCAUSE_EXTERNAL_IRQ:
			external_irq_handler();
			break;
		}
	} else {
		switch (excode) {
		case SCAUSE_EXCEPTION_INSTURCTION_ADDRESS_MISALIGNED:
			panic("SCAUSE_EXCEPTION_INSTURCTION_ADDRESS_MISALIGNED");
		case SCAUSE_EXCEPTION_INSTURCTION_ACCESSS_FAULT:
			panic("SCAUSE_EXCEPTION_INSTURCTION_ACCESSS_FAULT");
		case SCAUSE_EXCEPTION_ILLEGAL_INSTURCTION:
			panic("SCAUSE_EXCEPTION_ILLEGAL_INSTURCTION");
		case SCAUSE_EXCEPTION_BREAKPOINT:
			panic("SCAUSE_EXCEPTION_BREAKPOINT");
		case SCAUSE_EXCEPTION_LOAD_ADDRESS_MISALIGNED:
			panic("SCAUSE_EXCEPTION_LOAD_ADDRESS_MISALIGNED");
		case SCAUSE_EXCEPTION_LOAD_ACCESS_FAULT:
			panic("SCAUSE_EXCEPTION_LOAD_ACCESS_FAULT");
		case SCAUSE_EXCEPTION_STORE_ADDRESS_MISALIGNED:
			panic("SCAUSE_EXCEPTION_STORE_ADDRESS_MISALIGNED");
		case SCAUSE_EXCEPTION_STORE_ACCESS_FAULT:
			panic("SCAUSE_EXCEPTION_STORE_ACCESS_FAULT");
		case SCAUSE_EXCEPTION_ENVIRONMENT_CALL_FROM_UMODE:
			panic("SCAUSE_EXCEPTION_ENVIRONMENT_CALL_FROM_UMODE");
		case SCAUSE_EXCEPTION_ENVIRONMENT_CALL_FROM_SMODE:
			panic("SCAUSE_EXCEPTION_ENVIRONMENT_CALL_FROM_SMODE");
		case SCAUSE_EXCEPTION_INSTURCTION_PAGE_FAULT:
			panic("SCAUSE_EXCEPTION_INSTURCTION_PAGE_FAULT");
		case SCAUSE_EXCEPTION_LOAD_PAGE_FAULT:
			panic("SCAUSE_EXCEPTION_LOAD_PAGE_FAULT");
		case SCAUSE_EXCEPTION_STORE_PAGE_FAULT:
			panic("SCAUSE_EXCEPTION_STORE_PAGE_FAULT");
		}
	}

	irq_cnt[cpuid()]++;
}

static void user_timer_irq_handler(void)
{
	/* check for panic and spin if true */
	while (paniced);

	/* We need to disable stie bit or timer interrupt
	 * will be triggered again immediately after return
	 * because stip bit is always enabled
	 */
	w_sie(r_sie() & ~SIE_STIE);

	/* Interrupts will be disabled on sret
	 * because we must hold proccess lock
	 */
	irq_cnt[cpuid()]++;
	spinlock_acquire(&curproc()->lock);
	w_sstatus((r_sstatus() | SSTATUS_SPP_S) & ~SSTATUS_SPIE);

	/* route interrupts to kernel_irq_handler */
	w_stvec((u64) kerneltrap | STVEC_MODE_DIRECT);

	/* copy trapframe saved registers to pcontext */
	curproc()->context->ra = curproc()->trapframe->ra;
	curproc()->context->sp = curproc()->trapframe->sp;
	curproc()->context->gp = curproc()->trapframe->gp;
	curproc()->context->tp = curproc()->trapframe->tp;
	curproc()->context->t0 = curproc()->trapframe->t0;
	curproc()->context->t1 = curproc()->trapframe->t1;
	curproc()->context->t2 = curproc()->trapframe->t2;
	curproc()->context->fp = curproc()->trapframe->fp;
	curproc()->context->s1 = curproc()->trapframe->s1;
	curproc()->context->a0 = curproc()->trapframe->a0;
	curproc()->context->a1 = curproc()->trapframe->a1;
	curproc()->context->a2 = curproc()->trapframe->a2;
	curproc()->context->a3 = curproc()->trapframe->a3;
	curproc()->context->a4 = curproc()->trapframe->a4;
	curproc()->context->a5 = curproc()->trapframe->a5;
	curproc()->context->a6 = curproc()->trapframe->a6;
	curproc()->context->a7 = curproc()->trapframe->a7;
	curproc()->context->s2 = curproc()->trapframe->s2;
	curproc()->context->s3 = curproc()->trapframe->s3;
	curproc()->context->s4 = curproc()->trapframe->s4;
	curproc()->context->s5 = curproc()->trapframe->s5;
	curproc()->context->s6 = curproc()->trapframe->s6;
	curproc()->context->s7 = curproc()->trapframe->s7;
	curproc()->context->s8 = curproc()->trapframe->s8;
	curproc()->context->s9 = curproc()->trapframe->s9;
	curproc()->context->s10 = curproc()->trapframe->s10;
	curproc()->context->s11 = curproc()->trapframe->s11;
	curproc()->context->t3 = curproc()->trapframe->t3;
	curproc()->context->t4 = curproc()->trapframe->t4;
	curproc()->context->t5 = curproc()->trapframe->t5;
	curproc()->context->t6 = curproc()->trapframe->t6;

	/* save process epc */
	curproc()->context->epc = r_sepc();

	/* we must return after switch_to_user call in scheduler */
	w_sepc((u64) curcpu()->context->ra);

	/* return to scheduler */
	switch_to_scheduler(curcpu()->context);
}

void user_irq_handler(void)
{
	u64 scause = r_scause();
	u64 intr = scause & SCAUSE_INTERRUPT_MASK;
	u64 excode = scause & SCAUSE_EXCEPTION_CODE_MASK;

	/* interrupts are off so decrement irq_cnt */
	irq_cnt[cpuid()]--;

	if (intr) {
		switch (excode) {
		case SCAUSE_TIMER_IRQ:
			user_timer_irq_handler();		
		case SCAUSE_SOFTWARE_IRQ:
			panic("SCAUSE_SOFTWARE_IRQ");
		case SCAUSE_EXTERNAL_IRQ:
			external_irq_handler();
			break;
		}
	} else {
		switch (excode) {
		case SCAUSE_EXCEPTION_INSTURCTION_ADDRESS_MISALIGNED:
			panic("SCAUSE_EXCEPTION_INSTURCTION_ADDRESS_MISALIGNED");
		case SCAUSE_EXCEPTION_INSTURCTION_ACCESSS_FAULT:
			panic("SCAUSE_EXCEPTION_INSTURCTION_ACCESSS_FAULT");
		case SCAUSE_EXCEPTION_ILLEGAL_INSTURCTION:
			panic("SCAUSE_EXCEPTION_ILLEGAL_INSTURCTION");
		case SCAUSE_EXCEPTION_BREAKPOINT:
			panic("SCAUSE_EXCEPTION_BREAKPOINT");
		case SCAUSE_EXCEPTION_LOAD_ADDRESS_MISALIGNED:
			panic("SCAUSE_EXCEPTION_LOAD_ADDRESS_MISALIGNED");
		case SCAUSE_EXCEPTION_LOAD_ACCESS_FAULT:
			panic("SCAUSE_EXCEPTION_LOAD_ACCESS_FAULT");
		case SCAUSE_EXCEPTION_STORE_ADDRESS_MISALIGNED:
			panic("SCAUSE_EXCEPTION_STORE_ADDRESS_MISALIGNED");
		case SCAUSE_EXCEPTION_STORE_ACCESS_FAULT:
			panic("SCAUSE_EXCEPTION_STORE_ACCESS_FAULT");
		case SCAUSE_EXCEPTION_ENVIRONMENT_CALL_FROM_UMODE:
			syscall();
			break;
		case SCAUSE_EXCEPTION_ENVIRONMENT_CALL_FROM_SMODE:
			panic("SCAUSE_EXCEPTION_ENVIRONMENT_CALL_FROM_SMODE");
		case SCAUSE_EXCEPTION_INSTURCTION_PAGE_FAULT:
			panic("SCAUSE_EXCEPTION_INSTURCTION_PAGE_FAULT");
		case SCAUSE_EXCEPTION_LOAD_PAGE_FAULT:
			panic("SCAUSE_EXCEPTION_LOAD_PAGE_FAULT");
		case SCAUSE_EXCEPTION_STORE_PAGE_FAULT:
			panic("SCAUSE_EXCEPTION_STORE_PAGE_FAULT");
		}
	}

	irq_cnt[cpuid()]++;

	/* put process pagetable addr into sscratch for usertrap */
	w_sscratch(PA_TO_PN(curproc()->pagetable) | SATP_MODE_SV39);
}

void irq_init(void)
{
	for (size_t i = 0; i < NCPU; i++) {
		irq_cnt[i] = 0;
	}
}

void irq_hart_init(void)
{
	w_sie(SIE_SSIE | SIE_SEIE);
	w_stvec(((u64) kerneltrap) | STVEC_MODE_DIRECT);
}

