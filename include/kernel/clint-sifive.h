#ifndef KERNEL_CLINT_SIFIVE_H
#define KERNEL_CLINT_SIFIVE_H

#include <kernel/types.h>

#define CLINT_MTIME ((volatile u64 *) (VIRT_CLINT + 0xbff8))
#define CLINT_MTIMECMP(hartid) ((volatile u64 *) \
		(VIRT_CLINT + 0x4000 + 8 * (hartid)))

void clint_init(void);

#endif

