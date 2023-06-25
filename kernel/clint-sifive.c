#include <kernel/clint-sifive.h>
#include <kernel/proc.h>
#include <kernel/riscv64.h>

volatile uint64_t tscratch[NCPU][7];

void timertrap(void);

void clint_init(void)
{
	uint64_t mhartid = r_mhartid();
	volatile uint64_t *mtime = CLINT_MTIME;
	volatile uint64_t *mtimecmp = CLINT_MTIMECMP(r_mhartid());

	/* schedule next timer interrupt */
	*mtimecmp = *mtime + NCYCLE;

	/* We will use timer scratch in ram because
	 * only mscratch register is not enough
	 */
	tscratch[mhartid][0] = (uint64_t) mtime;
	tscratch[mhartid][1] = (uint64_t) mtimecmp;
	tscratch[mhartid][2] = NCYCLE;
	w_mscratch((uint64_t) &tscratch[mhartid]);

	/* set timertrap */
	w_mtvec(((uint64_t) timertrap) | MTVEC_MODE_DIRECT);

	/* enable timer interrupt */
	w_mip(MIP_STIP);
	w_mie(MIE_MTIE);

	/* set mpie to enable m-mode interrupts after mret */
	w_mstatus(r_mstatus() | MSTATUS_MPIE);
}

