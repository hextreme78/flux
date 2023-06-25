#ifndef KERNEL_TLB_H
#define KERNEL_TLB_H

#include <stdint.h>

static inline void tlb_flush_all(void)
{
	asm volatile("sfence.vma x0, x0");
}

static inline void tlb_flush_asid(uint64_t asid)
{
	asm volatile("sfence.vma x0, %0"
			:
			: "r" (asid));
}

static inline void tlb_flush_vaddr(uint64_t vaddr)
{
	asm volatile("sfence.vma %0, x0"
			:
			: "r" (vaddr));
}

#endif

