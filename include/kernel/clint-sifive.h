#ifndef KERNEL_CLINT_SIFIVE_H
#define KERNEL_CLINT_SIFIVE_H

#include <stdint.h>

#define CLINT_MTIME ((volatile uint64_t *) (VIRT_CLINT + 0xbff8))
#define CLINT_MTIMECMP(hartid) ((volatile uint64_t *) \
		(VIRT_CLINT + 0x4000 + 8 * (hartid)))

void clint_init(void);

#endif

