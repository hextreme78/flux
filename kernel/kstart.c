#include <kernel/riscv64.h>
#include <kernel/proc.h>
#include <kernel/clint-sifive.h>

void kmain(void);
__attribute__((aligned(16))) char kstack[KSTACKSIZE * NCPU];

void kstart(void)
{
	/* switching to s-mode */

	/* set mpp to s-mode for mret */
	w_mstatus((r_mstatus() & ~MSTATUS_MPP_MASK) | MSTATUS_MPP_S);

	/* delegate exceptions and interrupts to s-mode */
	w_medeleg(0xbbff);
	w_mideleg(0x1eee);

	/* set kmain as return address for mret */
	w_mepc((uint64_t) kmain);

	/* configure pmp memory protection for s-mode */
	w_pmpaddr0(0x3fffffffffffff);
	w_pmpcfg0(PMPXCFG(0, PMPXCFG_R | PMPXCFG_W | PMPXCFG_X | PMPXCFG_A_TOR));

	/* save hart id in tp register. we can not access mhartid in s-mode */
	w_tp(r_mhartid());
	
	/* disable paging for s-mode */
	w_satp(0);

	/* init timer. we can do it only in m-mode */
	clint_init();

	/* return to kmain in s-mode */
	mret();
}

