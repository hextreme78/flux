#include <kernel/irq.h>
#include <kernel/sched.h>
#include <kernel/kprintf.h>
#include <kernel/riscv64.h>
#include <kernel/uart-ns16550a.h>
#include <kernel/plic-sifive.h>
#include <kernel/proc.h>
#include <kernel/syscall.h>
#include <kernel/virtio.h>

static void external_irq_handler(void)
{
	u32 irq = plic_irq_claim();
	switch (irq) {
	case VIRT_PLIC_UART0:
		uart_irq_handler();
		break;
	case VIRT_PLIC_VIRTIO0:
	case VIRT_PLIC_VIRTIO1:
	case VIRT_PLIC_VIRTIO2:
	case VIRT_PLIC_VIRTIO3:
	case VIRT_PLIC_VIRTIO4:
	case VIRT_PLIC_VIRTIO5:
	case VIRT_PLIC_VIRTIO6:
	case VIRT_PLIC_VIRTIO7:
		virtio_irq_handler(VIRT_PLIC_VIRTIO_DEVNUM(irq));
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

	/* switch to the next task */
	sched();
}

void user_irq_handler(void)
{
	u64 scause = r_scause();
	u64 intr = scause & SCAUSE_INTERRUPT_MASK;
	u64 excode = scause & SCAUSE_EXCEPTION_CODE_MASK;

	if (intr) {
		switch (excode) {
		case SCAUSE_TIMER_IRQ:
			user_timer_irq_handler();
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
}

void irq_hart_init(void)
{
	void kerneltrap(void);

	w_sie(SIE_SSIE | SIE_SEIE);
	w_stvec(((u64) kerneltrap) | STVEC_MODE_DIRECT);
}

