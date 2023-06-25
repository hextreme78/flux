#include <kernel/ram.h>
#include <kernel/riscv64.h>
#include <kernel/vm.h>

static volatile uint64_t ramstart = 0;
static volatile uint64_t ramend = 0;
uint64_t volatile raminitstop = 0;

extern uint64_t kend;

void ramtrap(void);

/* last valid free address */
uint64_t ram_end(void)
{
	return ramend;
}

/* first free address after kernel */
uint64_t ram_start(void)
{
	return ramstart;
}

uint64_t ram_size(void)
{
	return ramend - ramstart + 1;
}

/* alloc n bytes before alloc_init */
void *ram_steal(uint64_t sz)
{
	uint64_t tmp = ramstart;
	ramstart += sz;
	ramstart = PAGEROUND(ramstart);
	return (void *) tmp;
}

/* determine size of ram */
void ram_init(void)
{
	ramstart = (uint64_t) &kend;
	ramend = ramstart;

	/* ramtrap will set stop flag */
	w_stvec(((uint64_t) ramtrap) | STVEC_MODE_DIRECT);

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

