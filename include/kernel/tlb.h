#ifndef KERNEL_TLB_H
#define KERNEL_TLB_H

#include <kernel/types.h>

static inline void tlb_flush_all(void)
{
	asm volatile("sfence.vma x0, x0");
}

static inline void tlb_flush_asid(u64 asid)
{
	asm volatile("sfence.vma x0, %0"
			:
			: "r" (asid));
}

static inline void tlb_flush_vaddr(u64 vaddr)
{
	asm volatile("sfence.vma %0, x0"
			:
			: "r" (vaddr));
}

#endif

