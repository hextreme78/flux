#ifndef KERNEL_MEMLAYOUT_H
#define KERNEL_MEMLAYOUT_H

#define PAGESZ 4096

/* for sv39 paging */
#define VA_MAX ((1ull << (9 + 9 + 9 + 12 - 1)) - 1)

#define VA_TRAMPOLINE (VA_MAX - PAGESZ + 1)
#define VA_TRAPFRAME (VA_TRAMPOLINE - PAGESZ)
#define VA_USTACK (VA_TRAPFRAME - USTACKNPAGES * PAGESZ)
#define VA_USTACK_GUARD (VA_USTACK - PAGESZ)

#endif

