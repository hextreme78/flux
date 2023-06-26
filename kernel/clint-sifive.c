#include <kernel/clint-sifive.h>
#include <kernel/proc.h>
#include <kernel/riscv64.h>

volatile u64 tscratch[NCPU][7];

void timertrap(void);

void clint_init(void)
{
	u64 mhartid = r_mhartid();
	volatile u64 *mtime = CLINT_MTIME;
	volatile u64 *mtimecmp = CLINT_MTIMECMP(r_mhartid());

	/* schedule next timer interrupt */
	*mtimecmp = *mtime + NCYCLE;

	/* We will use timer scratch in ram because
	 * only mscratch register is not enough
	 */
	tscratch[mhartid][0] = (u64) mtime;
	tscratch[mhartid][1] = (u64) mtimecmp;
	tscratch[mhartid][2] = NCYCLE;
	w_mscratch((u64) &tscratch[mhartid]);

	/* set timertrap */
	w_mtvec(((u64) timertrap) | MTVEC_MODE_DIRECT);

	/* enable timer interrupt */
	w_mip(MIP_STIP);
	w_mie(MIE_MTIE);

	/* set mpie to enable m-mode interrupts after mret */
	w_mstatus(r_mstatus() | MSTATUS_MPIE);
}

