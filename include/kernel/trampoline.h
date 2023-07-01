#ifndef KERNEL_TRAMPOLINE_H
#define KERNEL_TRAMPOLINE_H

#include <kernel/types.h>
#include <kernel/memlayout.h>

extern u64 trampoline;

void __trampoline_userret(void);
void __trampoline_usertrap(void);

#define TRAMPOLINE(symbol) (VA_TRAMPOLINE + (u64) (symbol) - (u64) &trampoline)
#define trampoline_userret ((void (*)(void)) TRAMPOLINE(__trampoline_userret))
#define trampoline_usertrap ((void (*)(void)) TRAMPOLINE(__trampoline_usertrap))

#endif

