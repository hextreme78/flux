#ifndef KERNEL_RAM_H
#define KERNEL_RAM_H

#include <stdint.h>

void ram_init(void);
uint64_t ram_end(void);
uint64_t ram_start(void);
uint64_t ram_size(void);
void *ram_steal(uint64_t sz);

#endif

