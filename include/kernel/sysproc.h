#ifndef KERNEL_SYSPROC_H
#define KERNEL_SYSPROC_H

#include <kernel/proc.h>

void sys__exit(int status);
u64 sys_wait(int *status);
u64 sys_getpid(void);

#endif

