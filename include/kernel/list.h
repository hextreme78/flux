#ifndef KERNEL_LIST_H
#define KERNEL_LIST_H

#include <kernel/types.h>

typedef struct list list_t;

#define list_entry(ptr, type, field) \
	((type *) ((u64) (ptr) - (u64) &((type *) 0)->field))

#define list_next_entry(ptr, field) \
	(list_entry((ptr)->field.next, typeof(*(ptr)), field))

#define list_prev_entry(ptr, field) \
	(list_entry((ptr)->field.prev, typeof(*(ptr)), field))

#define list_for_each_entry(pos, head, field) \
	for ((pos) = list_next_entry(head, field); \
			(pos) != (head); \
			(pos) = list_next_entry(pos, field))

struct list {
	list_t *next, *prev;
};

static inline void list_init(list_t *entry)
{
	entry->next = entry->prev = entry;
}

static inline void list_add(list_t *new, list_t *head)
{
	new->next = head->next;
	new->prev = head;
	head->next = new;
	new->next->prev = new;
}

static inline void list_add_tail(list_t *new, list_t *head)
{
	new->prev = head->prev;
	new->next = head;
	head->prev = new;
	new->prev->next = new;
}

static inline void list_del(list_t *entry)
{
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;
	entry->next = entry->prev = entry;
}

static inline bool list_empty(list_t *entry)
{
	return entry->next == entry;
}

static inline void list_splice(list_t *list, list_t *head)
{
	head->next->prev = list->prev;
	list->prev->next = head->next;
	head->next = list;
	list->prev = head;
}

#endif

