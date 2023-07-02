#ifndef KERNEL_ALLOC_H
#define KERNEL_ALLOC_H

#include <kernel/types.h>

typedef struct kpagemap kpagemap_t;
typedef struct suballoc suballoc_t;
typedef struct alloc    alloc_t;

#include <kernel/list.h>

#define ALIGNED_ALLOC_SZ(sz, align) ((sz) + (align) - 1)
#define ALIGNED_ALLOC_PTR(ptr, align) ((typeof(ptr)) \
		(((u64) (ptr) + (align) - 1) & ~((align) - 1)))

struct kpagemap {
	bool alloc : 1;
	bool last_alloc : 1;
} __attribute__((packed));

struct suballoc {
	bool alloc;
	size_t size;
	list_t suballoc_list;
	alloc_t *parent_alloc;
};

struct alloc {
	u64 npages;
	suballoc_t suballoc_head;
	list_t alloc_list;
};

void alloc_init(void);

void *kpage_alloc(size_t npages);
void kpage_free(void *mem);

__attribute__((alloc_size(1))) void *kmalloc(size_t sz);
void kfree(void *mem);

#endif

