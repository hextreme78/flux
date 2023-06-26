#ifndef KERNEL_RAM_H
#define KERNEL_RAM_H

#include <kernel/types.h>

void ram_init(void);
u64 ram_end(void);
u64 ram_start(void);
u64 ram_size(void);
void *ram_steal(u64 sz);

#endif

