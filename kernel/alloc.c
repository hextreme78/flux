#include <kernel/alloc.h>
#include <kernel/ram.h>
#include <kernel/vm.h>
#include <kernel/spinlock.h>
#include <kernel/klib.h>

static spinlock_t kpagemap_lock;
static kpagemap_t *kpagemap;
static uint64_t maxpages;

static spinlock_t kmalloc_lock;
static alloc_t kmalloc_list;

void alloc_init(void)
{
	uint64_t ramsz;

	spinlock_init(&kpagemap_lock);
	spinlock_init(&kmalloc_lock);

	/* we need n maxpages for ram size ramsz */
	ramsz = ram_size();
	maxpages = ramsz / PAGESZ;
	/* after ram_steal ramstart will be page-aligned */
	kpagemap = ram_steal(maxpages * sizeof(kpagemap_t));

	/* after ram_steal alloc ramsz became less */
	/* so we need to calculate maxpages again */
	ramsz = ram_size();
	maxpages = ramsz / PAGESZ;

	bzero(kpagemap, maxpages * sizeof(*kpagemap));

	list_init(&kmalloc_list.alloc_list);
}

void *kpage_alloc(size_t npages)
{
	void *paddr;
	if (maxpages < npages) {
		return NULL;
	}

	spinlock_acquire(&kpagemap_lock);
	for (size_t i = 0; i < maxpages - npages; i++) {
		if (!kpagemap[i].alloc) {
			/* check if next n - 1 pages are free */
			for (size_t j = i + 1; j < i + npages; j++) {
				if (kpagemap[j].alloc) {
					i = j;
					goto nextiter;
				}
			}

			/* alloc pages */
			for (size_t j = i; j < i + npages; j++) {
				kpagemap[j].alloc = true;
			}

			/* set last_alloc for last page */
			kpagemap[i + npages - 1].last_alloc = true;

			spinlock_release(&kpagemap_lock);

			paddr = (void *) (ram_start() + i * PAGESZ);

			bzero(paddr, npages * PAGESZ);

			return paddr;
		}
nextiter:
		;
	}

	spinlock_release(&kpagemap_lock);
	return NULL;
}

void kpage_free(void *mem)
{
	if ((uint64_t) mem < ram_start()) {
		return;
	}
	size_t kpagei = (((uint64_t) mem) - ram_start()) / PAGESZ;
	spinlock_acquire(&kpagemap_lock);
	while (1) {
		kpagemap[kpagei].alloc = false;
		if (kpagemap[kpagei].last_alloc) {
			kpagemap[kpagei].last_alloc = false;
			break;
		}
		kpagei++;
	}
	spinlock_release(&kpagemap_lock);
}

static size_t __kmalloc_free_size(alloc_t *alloc)
{
	size_t sz = sizeof(alloc_t);
	suballoc_t *suballoc;

	list_for_each_entry (suballoc, &alloc->suballoc_head, suballoc_list) {
		sz += suballoc->size + sizeof(suballoc_t);
	}

	sz = alloc->npages * PAGESZ - sz;

	return sz;
}

static void *__kmalloc_suballoc_existing_or_new(alloc_t *alloc, size_t memsz)
{
	suballoc_t *suballoc;

	if (__kmalloc_free_size(alloc) < memsz + sizeof(suballoc_t)) {
		return NULL;
	}

	list_for_each_entry (suballoc, &alloc->suballoc_head, suballoc_list) {
		if (suballoc->alloc) {
			continue;
		}
		if (suballoc->size == memsz) {
			/* return old suballoc */
			suballoc->alloc = true;
			return suballoc + 1;
		} else if (suballoc->size > memsz &&
				suballoc->size - memsz > sizeof(suballoc_t)) {
			/* make two suballocs from one and return left */
			suballoc_t *right, *left = suballoc;
			size_t suballocsz = suballoc->size;
			left->size = memsz;
			left->alloc = true;

			right = (suballoc_t *) ((uint8_t *) (left + 1) + suballocsz);
			right->size = suballocsz - memsz - sizeof(suballoc_t);
			right->alloc = false;
			right->parent_alloc = alloc;

			list_add(&right->suballoc_list, &left->suballoc_list);
			
			return left + 1;
		}
	}

	/* if there is no mem between suballocs try to add to the end of list */
	suballoc_t *last = list_prev_entry(&alloc->suballoc_head, suballoc_list);
	suballoc_t *new = (suballoc_t *) ((uint64_t) (last + 1) + last->size);
	if ((uint64_t) new + memsz <= alloc->npages * PAGESZ + (uint64_t) alloc) {
		new->alloc = true;
		new->size = memsz;
		new->parent_alloc = alloc;
		list_add_tail(&new->suballoc_list, &alloc->suballoc_head.suballoc_list);
		return new + 1;
	}

	return NULL;
}

static void *__kmalloc_alloc_and_suballoc_new(alloc_t *head, size_t memsz)
{
	uint64_t npages = PAGEROUND(memsz + sizeof(alloc_t) +
			sizeof(suballoc_t)) / PAGESZ;
	alloc_t *new_alloc = kpage_alloc(npages);
	if (!new_alloc) {
		return NULL;
	}
	new_alloc->npages = npages;
	list_init(&new_alloc->suballoc_head.suballoc_list);
	list_add(&new_alloc->alloc_list, &kmalloc_list.alloc_list);

	suballoc_t *new_suballoc = (suballoc_t *) (new_alloc + 1);
	new_suballoc->alloc = true;
	new_suballoc->size = memsz;
	new_suballoc->parent_alloc = new_alloc;
	list_add(&new_suballoc->suballoc_list, &new_alloc->suballoc_head.suballoc_list);

	return new_suballoc + 1;
}

void *kmalloc(size_t memsz)
{
	void *mem;
	alloc_t *alloc;

	if (!memsz) {
		return NULL;
	}

	spinlock_acquire(&kmalloc_lock);

	list_for_each_entry (alloc, &kmalloc_list, alloc_list) {
		mem = __kmalloc_suballoc_existing_or_new(alloc, memsz);
		if (mem) {
			spinlock_release(&kmalloc_lock);
			return mem;
		}
	}

	mem = __kmalloc_alloc_and_suballoc_new(&kmalloc_list, memsz);

	spinlock_release(&kmalloc_lock);

	return mem;
}

static suballoc_t *__kfree_merge_suballocs(suballoc_t *middle)
{
	alloc_t *alloc = middle->parent_alloc;
	suballoc_t *suballoc_head = &alloc->suballoc_head;
	suballoc_t *left = list_prev_entry(middle, suballoc_list);
	suballoc_t *right = list_next_entry(middle, suballoc_list);

	/* merge left and middle suballocs */
	if (left != suballoc_head && !left->alloc) {
		left->size += middle->size + sizeof(suballoc_t);
		list_del(&middle->suballoc_list);
		middle = left;
	}

	/* merge middle and right */
	if (right != suballoc_head && !right->alloc) {
		middle->size += right->size + sizeof(suballoc_t);
		list_del(&right->suballoc_list);
	}

	return middle;
}

static void __kfree_kpage_free(suballoc_t *suballoc)
{
	alloc_t *alloc = suballoc->parent_alloc;
	suballoc_t *left = list_prev_entry(suballoc, suballoc_list);
	suballoc_t *right = list_next_entry(suballoc, suballoc_list);

	/* if there is only one suballoc in alloc just free alloc pages */
	if (left == right) {
		list_del(&alloc->alloc_list);
		kpage_free(alloc);
	}
}

void kfree(void *mem)
{
	suballoc_t *suballoc = (suballoc_t *) mem - 1;

	if (!mem) {
		return;
	}

	spinlock_acquire(&kmalloc_lock);
	suballoc->alloc = false;
	suballoc = __kfree_merge_suballocs(suballoc);
	__kfree_kpage_free(suballoc);
	spinlock_release(&kmalloc_lock);
}

