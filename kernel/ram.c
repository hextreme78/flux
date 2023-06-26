#include <kernel/ram.h>
#include <kernel/riscv64.h>
#include <kernel/vm.h>

static volatile u64 ramstart = 0;
static volatile u64 ramend = 0;
u64 volatile raminitstop = 0;

extern u64 kend;

void ramtrap(void);

/* last valid free address */
u64 ram_end(void)
{
	return ramend;
}

/* first free address after kernel */
u64 ram_start(void)
{
	return ramstart;
}

u64 ram_size(void)
{
	return ramend - ramstart + 1;
}

/* alloc n bytes before alloc_init */
void *ram_steal(u64 sz)
{
	u64 tmp = ramstart;
	ramstart += sz;
	ramstart = PAGEROUND(ramstart);
	return (void *) tmp;
}

/* determine size of ram */
void ram_init(void)
{
	ramstart = (u64) &kend;
	ramend = ramstart;

	/* ramtrap will set stop flag */
	w_stvec(((u64) ramtrap) | STVEC_MODE_DIRECT);

	while (1) {
		/* try to access address */
		asm volatile("lb x0, (%0)"
				:
				: "r" (ramend + PAGESZ - 1));

		/* flag will be true if address is invalid */
		if (raminitstop) {
			break;
		}

		ramend = PAGEUP(ramend);
	}

	ramend -= 1;

	w_stvec(0);
}

