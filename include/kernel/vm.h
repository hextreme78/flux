#ifndef KERNEL_VM_H
#define KERNEL_VM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <kernel/memlayout.h>

#define PAGEDOWN(addr) ((((uint64_t) (addr)) / PAGESZ) * PAGESZ)
#define PAGEUP(addr) (PAGEDOWN(addr) + PAGESZ)
#define PAGEROUND(addr) (((uint64_t) (addr)) % PAGESZ ? PAGEUP(addr) : (uint64_t) (addr))
#define PA_TO_PN(addr) (((uint64_t) (addr)) >> 12)
#define PN_TO_PA(num) (((uint64_t) (num)) << 12)

#define VPN0_MASK (0x1ffull << 18)
#define VPN1_MASK (0x1ffull << 9)
#define VPN2_MASK 0x1ffull
#define VPN0(vpn) (((vpn) & VPN0_MASK) >> 18)
#define VPN1(vpn) (((vpn) & VPN1_MASK) >> 9)
#define VPN2(vpn) ((vpn) & VPN2_MASK)

#define PTE_MAX 512
#define PTE_RESET(ptr) (*(uint64_t *) (ptr) = 0)

#define PTE_V 1
#define PTE_R 2
#define PTE_W 4
#define PTE_X 8
#define PTE_U 16
#define PTE_G 32

typedef struct {
	uint64_t v : 1;
	uint64_t r : 1;
	uint64_t w : 1;
	uint64_t x : 1;
	uint64_t u : 1;
	uint64_t g : 1;
	uint64_t a : 1;
	uint64_t d : 1;
	uint64_t rsw : 2;
	uint64_t ppn : 44;
	uint64_t : 7;
	uint64_t pbmt : 2;
	uint64_t n : 1;
} __attribute__((packed)) pte_t;


void vm_init(void);
void vm_hart_init(void);

void vm_pagetable_init(pte_t *pagetable);
pte_t *vm_getpte(pte_t *pagetable0, size_t vpn);
bool vm_ismapped(pte_t *pagetable, size_t vpn);
void vm_pageunmap(pte_t *pagetable0, size_t vpn);
void vm_pageunmap_range(pte_t *pagetable, size_t vpn_first, size_t len);
void vm_pageunmap_all(pte_t *pagetable0);
int vm_pagemap(pte_t *pagetable0, uint8_t rwxug, size_t vpn, size_t ppn);
int vm_pagemap_range(pte_t *pagetable, uint8_t rwxug,
		size_t vpn_first, size_t ppn_first, size_t npages);

#endif

