#ifndef KERNEL_SYSPROC_H
#define KERNEL_SYSPROC_H

#include <kernel/proc.h>

void sys_exit(int status);
pid_t sys_wait(int *status);
pid_t sys_getpid(void);

#endif

