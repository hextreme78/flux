#include <kernel/riscv64.h>
#include <kernel/trampoline.h>
#include <kernel/proc.h>

/* to enter process first time we must jump into userret */
void userret(void)
{
	/* set proc state to running */
	curproc()->state = PROC_STATE_RUNNING;

	/* release lock acquired in scheduler */
	spinlock_release(&curproc()->lock);

	/* save cpuid */
	curproc()->trapframe->cpuid = cpuid();

	/* we will enter u-mode and interrupts will be enabled */
	w_sstatus((r_sstatus() & ~SSTATUS_SPP) | SSTATUS_SPIE);

	/* set epc address */
	w_sepc(curproc()->trapframe->epc);

	/* set usertrap address */
	w_stvec((u64) trampoline_usertrap);

	/* jump into trampoline part of userret */
	trampoline_userret();
}

