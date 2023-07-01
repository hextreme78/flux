#ifndef KERNEL_VM_H
#define KERNEL_VM_H

#include <kernel/memlayout.h>
#include <kernel/types.h>

#define PAGEDOWN(addr) ((((u64) (addr)) / PAGESZ) * PAGESZ)
#define PAGEUP(addr) (PAGEDOWN(addr) + PAGESZ)
#define PAGEROUND(addr) (((u64) (addr)) % PAGESZ ? PAGEUP(addr) : (u64) (addr))
#define PA_TO_PN(addr) (((u64) (addr)) >> 12)
#define PN_TO_PA(num) (((u64) (num)) << 12)

#define VPN0_MASK (0x1ffull << 18)
#define VPN1_MASK (0x1ffull << 9)
#define VPN2_MASK 0x1ffull
#define VPN0(vpn) (((vpn) & VPN0_MASK) >> 18)
#define VPN1(vpn) (((vpn) & VPN1_MASK) >> 9)
#define VPN2(vpn) ((vpn) & VPN2_MASK)

#define PTE_MAX 512
#define PTE_RESET(ptr) (*(u64 *) (ptr) = 0)

#define PTE_V 1
#define PTE_R 2
#define PTE_W 4
#define PTE_X 8
#define PTE_U 16
#define PTE_G 32

typedef struct {
	u64 v : 1;
	u64 r : 1;
	u64 w : 1;
	u64 x : 1;
	u64 u : 1;
	u64 g : 1;
	u64 a : 1;
	u64 d : 1;
	u64 rsw : 2;
	u64 ppn : 44;
	u64 : 7;
	u64 pbmt : 2;
	u64 n : 1;
} __attribute__((packed)) pte_t;


void vm_init(void);
void vm_hart_init(void);

void vm_pagetable_init(pte_t *pagetable);
pte_t *vm_getpte(pte_t *pagetable0, size_t vpn);
bool vm_ismapped(pte_t *pagetable, size_t vpn);
void vm_pageunmap(pte_t *pagetable0, size_t vpn);
void vm_pageunmap_range(pte_t *pagetable, size_t vpn_first, size_t len);
void vm_pageunmap_all(pte_t *pagetable0);
int vm_pagemap(pte_t *pagetable0, u8 rwxug, size_t vpn, size_t ppn);
int vm_pagemap_range(pte_t *pagetable, u8 rwxug,
		size_t vpn_first, size_t ppn_first, size_t npages);

int vm_pagemap_kpagetable(pte_t *kpagetable);
void vm_pageunmap_kpagetable(pte_t *kpagetable);
#endif

