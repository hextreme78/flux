#ifndef KERNEL_ALLOC_H
#define KERNEL_ALLOC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <kernel/list.h>

#define ALIGNED_ALLOC_SZ(sz, align) ((sz) + (align) - 1)
#define ALIGNED_ALLOC_PTR(ptr, align) ((typeof(ptr)) \
		(((uint64_t) (ptr) + (align) - 1) & ~((align) - 1)))

typedef struct {
	bool alloc : 1;
	bool last_alloc : 1;
} __attribute__((packed)) kpagemap_t;

typedef struct alloc alloc_t;

typedef struct {
	bool alloc;
	size_t size;
	list_t suballoc_list;
	alloc_t *parent_alloc;
} suballoc_t;

struct alloc {
	uint64_t npages;
	suballoc_t suballoc_head;
	list_t alloc_list;
};

void alloc_init(void);

void *kpage_alloc(size_t npages);
void kpage_free(void *mem);

__attribute__((alloc_size(1))) void *kmalloc(size_t sz);
void kfree(void *mem);

#endif

